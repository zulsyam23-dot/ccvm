//! Borrow checker untuk Rust-specific memory safety
//!
//! Modul ini bertanggung jawab untuk:
//! - Mutable vs immutable borrow checking
//! - Borrow lifetime validation
//! - Move semantics enforcement
//! - Copy vs Clone semantics
//! - Reference lifetime checking

use crate::{SemanticError, SemanticResult};
use std::collections::{HashMap, HashSet};
use super::lifetime::{BorrowKind, BorrowInfo, Lifetime, SourceLocation};
use super::symbol_table::SymbolTable;

/// Status dari variable dalam borrow checker
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum BorrowState {
    /// Variable belum diinisialisasi
    Uninitialized,
    /// Variable memiliki owned value
    Owned,
    /// Variable sudah di-move
    Moved,
    /// Variable sedang diborrow (dengan informasi borrow)
    Borrowed(Vec<BorrowInfo>),
}

/// Informasi tentang borrow yang aktif
#[derive(Debug, Clone)]
pub struct ActiveBorrow {
    pub borrow_id: usize,
    pub variable: String,
    pub kind: BorrowKind,
    pub lifetime: Lifetime,
    pub location: SourceLocation,
    pub is_used: bool,
}

/// Error yang spesifik untuk borrow checker
#[derive(Debug, Clone, PartialEq)]
pub enum BorrowCheckerError {
    MoveError { name: String, location: SourceLocation },
    BorrowConflict { name: String, kind: BorrowKind, location: SourceLocation },
    LifetimeError { name: String, reason: String, location: SourceLocation },
    BorrowOfUninitialized {
        variable: String,
        location: SourceLocation,
    },
    BorrowOfMoved {
        variable: String,
        location: SourceLocation,
    },
    ImmutableBorrowWithMutable {
        variable: String,
        location: SourceLocation,
    },
    MutableBorrowWithExisting {
        variable: String,
        existing_borrows: usize,
        location: SourceLocation,
    },
    MoveOfUninitialized {
        variable: String,
        location: SourceLocation,
    },
    MoveWhileBorrowed {
        variable: String,
        location: SourceLocation,
    },
    UseOfUninitialized {
        variable: String,
        location: SourceLocation,
    },
    UseAfterMove {
        variable: String,
        location: SourceLocation,
    },
    MutationWhileImmutableBorrowed {
        variable: String,
        location: SourceLocation,
    },
}

/// Borrow checker state untuk function
#[derive(Debug)]
pub struct BorrowCheckerState {
    /// Mapping dari variable name ke borrow state
    variable_states: HashMap<String, BorrowState>,
    /// Semua active borrows
    active_borrows: HashMap<usize, ActiveBorrow>,
    /// Move tracking
    moved_variables: HashSet<String>,
    /// Copy semantics tracking
    copyable_types: HashSet<String>,
    /// Clone semantics tracking
    clonable_types: HashSet<String>,
    /// Next borrow ID
    next_borrow_id: usize,
    /// Errors yang ditemukan
    errors: Vec<BorrowCheckerError>,
}

impl BorrowCheckerState {
    pub fn new() -> Self {
        let mut copyable_types = HashSet::new();
        copyable_types.insert("i32".to_string());
        copyable_types.insert("i64".to_string());
        copyable_types.insert("u32".to_string());
        copyable_types.insert("u64".to_string());
        copyable_types.insert("f32".to_string());
        copyable_types.insert("f64".to_string());
        copyable_types.insert("bool".to_string());
        copyable_types.insert("char".to_string());

        Self {
            variable_states: HashMap::new(),
            active_borrows: HashMap::new(),
            moved_variables: HashSet::new(),
            copyable_types,
            clonable_types: HashSet::new(),
            next_borrow_id: 0,
            errors: Vec::new(),
        }
    }

    /// Inisialisasi variable baru
    pub fn initialize_variable(&mut self, name: &str, var_type: &str, _location: &SourceLocation) {
        let state = if self.is_copyable(var_type) {
            BorrowState::Owned
        } else {
            BorrowState::Uninitialized
        };
        
        self.variable_states.insert(name.to_string(), state);
    }

    /// Assign value ke variable
    pub fn assign_variable(&mut self, name: &str, _value: &str, _location: &SourceLocation) {
        // TODO: Implementasi assignment yang lebih sophisticated
        // Untuk sekarang, asumsikan assignment sederhana
        
        self.variable_states.insert(name.to_string(), BorrowState::Owned);
    }

    /// Borrow variable
    pub fn borrow_variable(&mut self, name: &str, kind: BorrowKind, lifetime: Lifetime, location: SourceLocation) -> SemanticResult<usize> {
        // Cek apakah variable ada
        let state = self.variable_states.get(name)
            .ok_or_else(|| SemanticError::BorrowChecker(
                format!("Variable '{}' not found", name)
            ))?;

        match state {
            BorrowState::Uninitialized => {
                self.errors.push(BorrowCheckerError::BorrowOfUninitialized {
                    variable: name.to_string(),
                    location: location.clone(),
                });
                return Err(SemanticError::BorrowChecker(
                    format!("Cannot borrow uninitialized variable '{}'", name)
                ));
            }
            BorrowState::Moved => {
                self.errors.push(BorrowCheckerError::BorrowOfMoved {
                    variable: name.to_string(),
                    location: location.clone(),
                });
                return Err(SemanticError::BorrowChecker(
                    format!("Cannot borrow moved variable '{}'", name)
                ));
            }
            BorrowState::Borrowed(existing_borrows) => {
                // Validasi borrow rules
                self.validate_borrow_rules(name, kind, existing_borrows, &location)?;
            }
            BorrowState::Owned => {
                // OK untuk borrow owned value
            }
        }

        // Cek conflict dengan active borrows
        let active_borrows = self.get_active_borrows_for_variable(name);
        if let Some(active) = active_borrows {
            self.validate_borrow_conflict(name, kind, active, &location)?;
        }

        // Buat borrow baru
        let borrow_id = self.next_borrow_id;
        self.next_borrow_id += 1;

        let borrow_info = BorrowInfo {
            lifetime: lifetime.clone(),
            kind,
            borrowed_from: name.to_string(),
            location: location.clone(),
        };

        let active_borrow = ActiveBorrow {
            borrow_id,
            variable: name.to_string(),
            kind,
            lifetime,
            location,
            is_used: false,
        };

        // Update variable state
        let new_state = match self.variable_states.get(name) {
            Some(BorrowState::Owned) => BorrowState::Borrowed(vec![borrow_info]),
            Some(BorrowState::Borrowed(existing)) => {
                let mut new_borrows = existing.clone();
                new_borrows.push(borrow_info);
                BorrowState::Borrowed(new_borrows)
            }
            _ => return Err(SemanticError::BorrowChecker(
                format!("Invalid borrow state for variable '{}'", name)
            )),
        };

        self.variable_states.insert(name.to_string(), new_state);
        self.active_borrows.insert(borrow_id, active_borrow);

        Ok(borrow_id)
    }

    /// Validasi borrow rules
    fn validate_borrow_rules(&self, name: &str, kind: BorrowKind, existing_borrows: &[BorrowInfo], _location: &SourceLocation) -> SemanticResult<()> {
        match kind {
            BorrowKind::Immutable => {
                // Immutable borrow OK jika semua existing borrows juga immutable
                for borrow in existing_borrows {
                    if borrow.kind == BorrowKind::Mutable {
                        return Err(SemanticError::BorrowChecker(
                            format!("Cannot immutably borrow '{}' while mutably borrowed", name)
                        ));
                    }
                }
                Ok(())
            }
            BorrowKind::Mutable => {
                // Mutable borrow hanya OK jika tidak ada existing borrows
                if !existing_borrows.is_empty() {
                    return Err(SemanticError::BorrowChecker(
                        format!("Cannot mutably borrow '{}' while borrowed", name)
                    ));
                }
                Ok(())
            }
        }
    }

    /// Validasi conflict dengan active borrows
    fn validate_borrow_conflict(&mut self, name: &str, kind: BorrowKind, active_borrows: Vec<ActiveBorrow>, location: &SourceLocation) -> SemanticResult<()> {
        match kind {
            BorrowKind::Immutable => {
                // Immutable borrow OK jika semua active borrows juga immutable
                for borrow in &active_borrows {
                    if borrow.kind == BorrowKind::Mutable {
                        self.errors.push(BorrowCheckerError::ImmutableBorrowWithMutable {
                            variable: name.to_string(),
                            location: location.clone(),
                        });
                        return Err(SemanticError::BorrowChecker(
                            format!("Cannot immutably borrow '{}' while mutably borrowed", name)
                        ));
                    }
                }
                Ok(())
            }
            BorrowKind::Mutable => {
                // Mutable borrow hanya OK jika tidak ada active borrows
                if !active_borrows.is_empty() {
                    self.errors.push(BorrowCheckerError::MutableBorrowWithExisting {
                        variable: name.to_string(),
                        existing_borrows: active_borrows.len(),
                        location: location.clone(),
                    });
                    return Err(SemanticError::BorrowChecker(
                        format!("Cannot mutably borrow '{}' while {} borrows are active", name, active_borrows.len())
                    ));
                }
                Ok(())
            }
        }
    }

    /// Mendapatkan semua active borrows untuk variable tertentu
    fn get_active_borrows_for_variable(&self, name: &str) -> Option<Vec<ActiveBorrow>> {
        let result: Vec<ActiveBorrow> = self.active_borrows
            .values()
            .filter(|b| b.variable == name)
            .cloned()
            .collect();

        if result.is_empty() {
            None
        } else {
            Some(result)
        }
    }

    /// Move variable (transfer ownership)
    pub fn move_variable(&mut self, name: &str, location: &SourceLocation) -> SemanticResult<()> {
        let state = self.variable_states.get(name)
            .ok_or_else(|| SemanticError::BorrowChecker(
                format!("Variable '{}' not found", name)
            ))?;

        match state {
            BorrowState::Uninitialized => {
                self.errors.push(BorrowCheckerError::MoveOfUninitialized {
                    variable: name.to_string(),
                    location: location.clone(),
                });
                return Err(SemanticError::BorrowChecker(
                    format!("Cannot move uninitialized variable '{}'", name)
                ));
            }
            BorrowState::Moved => {
                self.errors.push(BorrowCheckerError::UseAfterMove {
                    variable: name.to_string(),
                    location: location.clone(),
                });
                return Err(SemanticError::BorrowChecker(
                    format!("Cannot use moved variable '{}'", name)
                ));
            }
            BorrowState::Borrowed(_) => {
                self.errors.push(BorrowCheckerError::MoveWhileBorrowed {
                    variable: name.to_string(),
                    location: location.clone(),
                });
                return Err(SemanticError::BorrowChecker(
                    format!("Cannot move borrowed variable '{}'", name)
                ));
            }
            BorrowState::Owned => {
                // OK untuk move owned value
            }
        }

        self.variable_states.insert(name.to_string(), BorrowState::Moved);
        self.moved_variables.insert(name.to_string());

        Ok(())
    }

    /// Use variable (read atau write)
    pub fn use_variable(&mut self, name: &str, is_mutation: bool, location: &SourceLocation) -> SemanticResult<()> {
        let state = self.variable_states.get(name)
            .ok_or_else(|| SemanticError::BorrowChecker(
                format!("Variable '{}' not found", name)
            ))?;

        match state {
            BorrowState::Uninitialized => {
                self.errors.push(BorrowCheckerError::UseOfUninitialized {
                    variable: name.to_string(),
                    location: location.clone(),
                });
                return Err(SemanticError::BorrowChecker(
                    format!("Cannot use uninitialized variable '{}'", name)
                ));
            }
            BorrowState::Moved => {
                self.errors.push(BorrowCheckerError::UseAfterMove {
                    variable: name.to_string(),
                    location: location.clone(),
                });
                return Err(SemanticError::BorrowChecker(
                    format!("Cannot use moved variable '{}'", name)
                ));
            }
            BorrowState::Borrowed(borrows) => {
                if is_mutation {
                    // Cek apakah ada immutable borrows
                    for borrow in borrows {
                        if borrow.kind == BorrowKind::Immutable {
                            self.errors.push(BorrowCheckerError::MutationWhileImmutableBorrowed {
                                variable: name.to_string(),
                                location: location.clone(),
                            });
                            return Err(SemanticError::BorrowChecker(
                                format!("Cannot mutate '{}' while immutably borrowed", name)
                            ));
                        }
                    }
                }
            }
            BorrowState::Owned => {
                // OK untuk use owned value
            }
        }

        Ok(())
    }

    /// End borrow (ketika lifetime berakhir)
    pub fn end_borrow(&mut self, borrow_id: usize) -> SemanticResult<()> {
        let borrow = self.active_borrows.remove(&borrow_id)
            .ok_or_else(|| SemanticError::BorrowChecker(
                format!("Borrow {} not found", borrow_id)
            ))?;

        // Update variable state
        if let Some(state) = self.variable_states.get_mut(&borrow.variable) {
            match state {
                BorrowState::Borrowed(borrows) => {
                    borrows.retain(|b| {
                        // Hapus borrow yang berakhir berdasarkan lifetime atau ID
                        // In real implementation, we would compare lifetimes
                        b.borrowed_from != borrow.variable
                    });
                    
                    if borrows.is_empty() {
                        *state = BorrowState::Owned;
                    }
                }
                _ => {
                    return Err(SemanticError::BorrowChecker(
                        format!("Invalid state for variable '{}'", borrow.variable)
                    ));
                }
            }
        }

        Ok(())
    }

    /// Cek apakah type copyable
    fn is_copyable(&self, var_type: &str) -> bool {
        self.copyable_types.contains(var_type)
    }

    /// Cek apakah type clonable
    fn is_clonable(&self, var_type: &str) -> bool {
        self.clonable_types.contains(var_type)
    }

    /// Get borrow checker errors
    pub fn get_errors(&self) -> &[BorrowCheckerError] {
        &self.errors
    }

    /// Clear errors
    pub fn clear_errors(&mut self) {
        self.errors.clear();
    }

    /// Get current state summary
    pub fn get_state_summary(&self) -> BorrowCheckerSummary {
        BorrowCheckerSummary {
            total_variables: self.variable_states.len(),
            moved_variables: self.moved_variables.len(),
            active_borrows: self.active_borrows.len(),
            errors: self.errors.len(),
        }
    }
}

/// Summary dari borrow checker state
#[derive(Debug, Clone)]
pub struct BorrowCheckerSummary {
    pub total_variables: usize,
    pub moved_variables: usize,
    pub active_borrows: usize,
    pub errors: usize,
}

/// Borrow Checker
pub struct BorrowChecker {
    state: BorrowCheckerState,
}

impl BorrowChecker {
    pub fn new() -> Self {
        Self {
            state: BorrowCheckerState::new(),
        }
    }

    /// Jalankan borrow checking untuk function
    pub fn check_function(&mut self, _func_name: &str, _symbol_table: &SymbolTable) -> SemanticResult<BorrowCheckerSummary> {
        // TODO: Implementasi borrow checking yang sebenarnya
        // berdasarkan IR dan symbol table
        
        // Untuk sekarang, return summary dummy
        Ok(self.state.get_state_summary())
    }

    /// Get mutable state untuk manual checking
    pub fn get_state_mut(&mut self) -> &mut BorrowCheckerState {
        &mut self.state
    }

    /// Get state untuk inspection
    pub fn get_state(&self) -> &BorrowCheckerState {
        &self.state
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_borrow_checker_state_creation() {
        let state = BorrowCheckerState::new();
        assert_eq!(state.active_borrows.len(), 0);
        assert_eq!(state.moved_variables.len(), 0);
        assert_eq!(state.errors.len(), 0);
    }

    #[test]
    fn test_variable_initialization() {
        let mut state = BorrowCheckerState::new();
        let location = SourceLocation {
            file: "test.rs".to_string(),
            line: 1,
            column: 1,
        };

        state.initialize_variable("x", "i32", &location);
        assert!(state.variable_states.contains_key("x"));
    }

    #[test]
    fn test_borrow_rules() {
        let mut state = BorrowCheckerState::new();
        let location = SourceLocation {
            file: "test.rs".to_string(),
            line: 1,
            column: 1,
        };

        // Initialize variable
        state.initialize_variable("x", "i32", &location);

        // Immutable borrow should work
        let result1 = state.borrow_variable("x", BorrowKind::Immutable, Lifetime::Static, location.clone());
        assert!(result1.is_ok());

        // Another immutable borrow should work
        let result2 = state.borrow_variable("x", BorrowKind::Immutable, Lifetime::Static, location.clone());
        assert!(result2.is_ok());

        // Mutable borrow should fail (ada immutable borrows)
        let result3 = state.borrow_variable("x", BorrowKind::Mutable, Lifetime::Static, location.clone());
        assert!(result3.is_err());
    }

    #[test]
    fn test_move_semantics() {
        let mut state = BorrowCheckerState::new();
        let location = SourceLocation {
            file: "test.rs".to_string(),
            line: 1,
            column: 1,
        };

        // Initialize variable
        state.initialize_variable("x", "String", &location);
        state.assign_variable("x", "hello", &location);

        // Move should work
        let result1 = state.move_variable("x", &location);
        assert!(result1.is_ok());

        // Use after move should fail
        let result2 = state.use_variable("x", false, &location);
        assert!(result2.is_err());
    }
}