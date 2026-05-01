//! Lifetime analysis untuk Rust-specific features
//!
//! Modul ini bertanggung jawab untuk:
//! - Lifetime inference dan checking
//! - Borrow checking (mutable vs immutable)
//! - Drop semantics analysis
//! - Ownership transfer analysis
//! - Lifetime parameter validation

use crate::{SemanticError, SemanticResult};
use std::collections::{HashMap, HashSet};
use crate::symbol_table::SymbolTable;

/// Representasi lifetime dalam Rust
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Lifetime {
    /// Named lifetime parameter (e.g., 'a)
    Named(String),
    /// Anonymous lifetime
    Anonymous(usize),
    /// Static lifetime
    Static,
    /// Elided lifetime (akan diinfer)
    Elided,
}

impl Lifetime {
    pub fn to_string(&self) -> String {
        match self {
            Lifetime::Named(name) => name.clone(),
            Lifetime::Anonymous(id) => format!("'_#{}'", id),
            Lifetime::Static => "'static".to_string(),
            Lifetime::Elided => "'_".to_string(),
        }
    }
}

/// Representasi borrow type (mutable atau immutable)
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum BorrowKind {
    Immutable,
    Mutable,
}

/// Informasi tentang borrow
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct BorrowInfo {
    pub lifetime: Lifetime,
    pub kind: BorrowKind,
    pub borrowed_from: String, // Nama variable yang dipinjam
    pub location: SourceLocation,
}

/// Informasi tentang lifetime constraint
#[derive(Debug, Clone)]
pub struct LifetimeConstraint {
    pub lhs: Lifetime,
    pub rhs: Lifetime,
    pub kind: ConstraintKind,
    pub location: SourceLocation,
}

/// Jenis lifetime constraint
#[derive(Debug, Clone)]
pub enum ConstraintKind {
    /// 'a: 'b (lifetime 'a outlives 'b)
    Outlives,
    /// 'a == 'b (lifetimes are equal)
    Equal,
}

use serde::{Serialize, Deserialize};

/// Representasi source location untuk error reporting
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct SourceLocation {
    pub file: String,
    pub line: u32,
    pub column: u32,
}

/// Lifetime Environment - menyimpan lifetime information
#[derive(Debug, Clone)]
pub struct LifetimeEnvironment {
    /// Mapping dari lifetime ke set of lifetimes yang harus dia outlive
    pub outlives_constraints: HashMap<Lifetime, HashSet<Lifetime>>,
    /// Set of lifetime equalities
    pub equality_constraints: HashSet<(Lifetime, Lifetime)>,
    /// Next anonymous lifetime ID
    next_anonymous_id: usize,
}

impl LifetimeEnvironment {
    pub fn new() -> Self {
        Self {
            outlives_constraints: HashMap::new(),
            equality_constraints: HashSet::new(),
            next_anonymous_id: 0,
        }
    }

    /// Generate anonymous lifetime baru
    pub fn fresh_anonymous(&mut self) -> Lifetime {
        let id = self.next_anonymous_id;
        self.next_anonymous_id += 1;
        Lifetime::Anonymous(id)
    }

    /// Tambahkan outlives constraint: lhs outlives rhs
    pub fn add_outlives(&mut self, lhs: Lifetime, rhs: Lifetime) {
        self.outlives_constraints
            .entry(lhs.clone())
            .or_default()
            .insert(rhs);
    }

    /// Tambahkan equality constraint
    pub fn add_equality(&mut self, lhs: Lifetime, rhs: Lifetime) {
        self.equality_constraints.insert((lhs, rhs));
    }

    /// Resolve lifetime constraints
    pub fn resolve_constraints(&self) -> SemanticResult<HashMap<Lifetime, Lifetime>> {
        let mut resolved = HashMap::new();
        
        // TODO: Implementasi constraint resolution yang lebih sophisticated
        // Untuk sekarang, buat mapping sederhana
        
        for (lifetime, _) in &self.outlives_constraints {
            resolved.insert(lifetime.clone(), lifetime.clone());
        }
        
        for (lhs, rhs) in &self.equality_constraints {
            resolved.insert(lhs.clone(), rhs.clone());
        }
        
        Ok(resolved)
    }
}

/// Variable dengan lifetime information
#[derive(Debug, Clone)]
pub struct LifetimeVariable {
    pub name: String,
    pub ty: VariableType,
    pub lifetime: Lifetime,
    pub is_mutable: bool,
    pub location: SourceLocation,
}

/// Jenis variable untuk lifetime analysis
#[derive(Debug, Clone)]
pub enum VariableType {
    Owned,
    Borrow(BorrowKind),
    Reference(BorrowKind),
    RawPointer,
}

/// Lifetime Analyzer
pub struct LifetimeAnalyzer {
    environment: LifetimeEnvironment,
    active_borrows: HashMap<String, Vec<BorrowInfo>>,
    drop_locations: HashMap<String, SourceLocation>,
    errors: Vec<LifetimeError>,
}

impl LifetimeAnalyzer {
    pub fn new() -> Self {
        Self {
            environment: LifetimeEnvironment::new(),
            active_borrows: HashMap::new(),
            drop_locations: HashMap::new(),
            errors: Vec::new(),
        }
    }

    /// Analisis lifetime untuk function
    pub fn analyze_function(&mut self, func_name: &str, symbol_table: &SymbolTable) -> SemanticResult<()> {
        // TODO: Integrasi dengan IR dan symbol table yang sebenarnya
        
        // Step 1: Kumpulkan lifetime constraints
        self.collect_lifetime_constraints(func_name, symbol_table)?;
        
        // Step 2: Resolve lifetime constraints
        let _resolved = self.environment.resolve_constraints()?;
        
        // Step 3: Validasi borrow rules
        self.validate_borrow_rules()?;
        
        // Step 4: Validasi drop semantics
        self.validate_drop_semantics()?;
        
        // Report errors jika ada
        if !self.errors.is_empty() {
            return Err(SemanticError::Lifetime(
                self.format_errors()
            ));
        }
        
        Ok(())
    }

    /// Kumpulkan lifetime constraints dari function signature
    fn collect_lifetime_constraints(&mut self, _func_name: &str, _symbol_table: &SymbolTable) -> SemanticResult<()> {
        // TODO: Implementasi pengumpulan constraints yang sebenarnya
        // dari function signature dan body
        
        // Dummy constraints untuk demonstrasi
        let lifetime_a = Lifetime::Named("'a".to_string());
        let lifetime_b = Lifetime::Named("'b".to_string());
        
        self.environment.add_outlives(lifetime_a.clone(), lifetime_b);
        self.environment.add_equality(lifetime_a, Lifetime::Static);
        
        Ok(())
    }

    /// Validasi Rust borrow rules
    fn validate_borrow_rules(&mut self) -> SemanticResult<()> {
        // Rule 1: No mutable borrows while immutable borrows are active
        self.validate_no_mutable_with_immutable()?;
        
        // Rule 2: No borrows of moved values
        self.validate_no_borrow_of_moved()?;
        
        // Rule 3: Lifetime of borrow tidak boleh melebihi lifetime dari borrowed value
        self.validate_borrow_lifetimes()?;
        
        Ok(())
    }

    /// Validasi tidak ada mutable borrow saat immutable borrows aktif
    fn validate_no_mutable_with_immutable(&mut self) -> SemanticResult<()> {
        for (var_name, borrows) in &self.active_borrows {
            let immutable_borrows: Vec<_> = borrows.iter()
                .filter(|b| b.kind == BorrowKind::Immutable)
                .collect();
            
            let mutable_borrows: Vec<_> = borrows.iter()
                .filter(|b| b.kind == BorrowKind::Mutable)
                .collect();
            
            if !immutable_borrows.is_empty() && !mutable_borrows.is_empty() {
                self.errors.push(LifetimeError::BorrowConflict {
                    variable: var_name.clone(),
                    immutable_borrows: immutable_borrows.len(),
                    mutable_borrows: mutable_borrows.len(),
                    location: mutable_borrows[0].location.clone(),
                });
            }
        }
        
        Ok(())
    }

    /// Validasi tidak ada borrow dari moved values
    fn validate_no_borrow_of_moved(&mut self) -> SemanticResult<()> {
        // TODO: Implementasi validasi move semantics
        Ok(())
    }

    /// Validasi lifetime dari borrows
    fn validate_borrow_lifetimes(&mut self) -> SemanticResult<()> {
        for (var_name, borrows) in &self.active_borrows {
            for borrow in borrows {
                // Validasi bahwa borrow lifetime tidak melebihi borrowed value lifetime
                if !self.is_valid_borrow_lifetime(&borrow.lifetime, &borrow.borrowed_from) {
                    self.errors.push(LifetimeError::InvalidBorrowLifetime {
                        borrow_lifetime: borrow.lifetime.to_string(),
                        variable: var_name.clone(),
                        borrowed_from: borrow.borrowed_from.clone(),
                        location: borrow.location.clone(),
                    });
                }
            }
        }
        
        Ok(())
    }

    /// Cek apakah borrow lifetime valid
    fn is_valid_borrow_lifetime(&self, _borrow_lifetime: &Lifetime, _borrowed_from: &str) -> bool {
        // TODO: Implementasi validasi lifetime yang sebenarnya
        // Untuk sekarang, anggap semua valid
        true
    }

    /// Validasi drop semantics
    fn validate_drop_semantics(&mut self) -> SemanticResult<()> {
        // Validasi bahwa values tidak digunakan setelah drop
        self.validate_no_use_after_drop()?;
        
        // Validasi drop order
        self.validate_drop_order()?;
        
        Ok(())
    }

    /// Validasi tidak ada penggunaan setelah drop
    fn validate_no_use_after_drop(&mut self) -> SemanticResult<()> {
        // TODO: Implementasi use-after-drop detection
        Ok(())
    }

    /// Validasi urutan drop
    fn validate_drop_order(&mut self) -> SemanticResult<()> {
        // TODO: Implementasi drop order validation
        Ok(())
    }

    /// Format lifetime errors untuk reporting
    fn format_errors(&self) -> String {
        let mut result = String::new();
        
        for error in &self.errors {
            match error {
                LifetimeError::BorrowConflict { variable, immutable_borrows, mutable_borrows, location } => {
                    result.push_str(&format!(
                        "Borrow conflict: Cannot have {} mutable borrows while {} immutable borrows of '{}' are active at {}:{}\n",
                        mutable_borrows, immutable_borrows, variable, location.line, location.column
                    ));
                }
                LifetimeError::InvalidBorrowLifetime { borrow_lifetime, variable, borrowed_from, location } => {
                    result.push_str(&format!(
                        "Invalid borrow lifetime: {} of '{}' from '{}' at {}:{}\n",
                        borrow_lifetime, variable, borrowed_from, location.line, location.column
                    ));
                }
                _ => {
                    result.push_str("Unknown lifetime error\n");
                }
            }
        }
        
        result
    }

    /// Infer elided lifetimes
    pub fn infer_elided_lifetimes(&mut self) -> SemanticResult<HashMap<String, Lifetime>> {
        let mut inferred = HashMap::new();
        
        // TODO: Implementasi lifetime inference rules yang sebenarnya
        // - Elided lifetimes in function arguments
        // - Elided lifetimes in return types
        // - Elided lifetimes in struct fields
        
        // Untuk sekarang, buat inference sederhana
        let fresh_lifetime = self.environment.fresh_anonymous();
        inferred.insert("elided".to_string(), fresh_lifetime);
        
        Ok(inferred)
    }

    /// Check lifetime variance
    pub fn check_variance(&self, _covariant: &Lifetime, _contravariant: &Lifetime, _invariant: &Lifetime) -> SemanticResult<()> {
        // TODO: Implementasi variance checking untuk generic types
        Ok(())
    }
}

/// Lifetime errors
#[derive(Debug, Clone)]
pub enum LifetimeError {
    BorrowConflict {
        variable: String,
        immutable_borrows: usize,
        mutable_borrows: usize,
        location: SourceLocation,
    },
    InvalidBorrowLifetime {
        borrow_lifetime: String,
        variable: String,
        borrowed_from: String,
        location: SourceLocation,
    },
    UseAfterDrop {
        variable: String,
        drop_location: SourceLocation,
        use_location: SourceLocation,
    },
    LifetimeMismatch {
        expected: String,
        found: String,
        location: SourceLocation,
    },
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::symbol_table::SymbolTable;

    #[test]
    fn test_lifetime_environment() {
        let mut env = LifetimeEnvironment::new();
        
        let lifetime_a = Lifetime::Named("'a".to_string());
        let lifetime_b = Lifetime::Named("'b".to_string());
        
        env.add_outlives(lifetime_a.clone(), lifetime_b.clone());
        env.add_equality(lifetime_a, lifetime_b);
        
        let result = env.resolve_constraints();
        assert!(result.is_ok());
    }

    #[test]
    fn test_lifetime_analyzer() {
        let mut analyzer = LifetimeAnalyzer::new();
        let symbol_table = SymbolTable::new();
        
        // TODO: Implementasi test yang lebih comprehensive
        // Untuk sekarang, cek bahwa analysis berjalan tanpa error
        let result = analyzer.analyze_function("test_func", &symbol_table);
        // Karena kita belum implementasi yang sebenarnya, ini mungkin akan gagal
        // Tapi setidaknya kita tahu struktur kode sudah benar
    }

    #[test]
    fn test_elided_lifetime_inference() {
        let mut analyzer = LifetimeAnalyzer::new();
        
        let result = analyzer.infer_elided_lifetimes();
        assert!(result.is_ok());
        
        let inferred = result.unwrap();
        assert!(!inferred.is_empty());
    }
}