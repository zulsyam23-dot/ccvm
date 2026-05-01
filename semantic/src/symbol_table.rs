//! Symbol table untuk semantic analysis

use std::collections::HashMap;
use std::sync::Arc;
use crate::{SymbolInfo, Result, SemanticError, SymbolType};
use parking_lot::RwLock;

pub use crate::types::SymbolInfo as Symbol;

/// Identifier untuk scope
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct ScopeId {
    id: usize,
}

impl ScopeId {
    pub fn global() -> Self {
        ScopeId { id: 0 }
    }
    
    pub fn new(id: usize) -> Self {
        ScopeId { id }
    }
    
    pub fn as_usize(&self) -> usize {
        self.id
    }
}

/// Jenis-jenis scope
#[derive(Debug, Clone, PartialEq)]
pub enum ScopeType {
    Global,
    Function,
    Block,
    Loop,
    Conditional,
    Namespace,
    Class,
    Trait,
    Implementation,
}

/// Informasi tentang scope
#[derive(Debug, Clone)]
pub struct Scope {
    pub id: ScopeId,
    pub scope_type: ScopeType,
    pub parent: Option<ScopeId>,
    pub symbols: HashMap<String, SymbolInfo>,
    pub children: Vec<ScopeId>,
}

impl Scope {
    pub fn new(id: ScopeId, scope_type: ScopeType, parent: Option<ScopeId>) -> Self {
        Scope {
            id,
            scope_type,
            parent,
            symbols: HashMap::new(),
            children: Vec::new(),
        }
    }
    
    pub fn insert(&mut self, name: String, symbol: SymbolInfo) {
        self.symbols.insert(name, symbol);
    }
    
    pub fn lookup(&self, name: &str) -> Option<&SymbolInfo> {
        self.symbols.get(name)
    }
    
    pub fn lookup_current(&self, name: &str) -> Option<&SymbolInfo> {
        self.symbols.get(name)
    }
    
    pub fn remove(&mut self, name: &str) -> Option<SymbolInfo> {
        self.symbols.remove(name)
    }
    
    pub fn get_symbols(&self) -> &HashMap<String, SymbolInfo> {
        &self.symbols
    }
    
    pub fn get_symbol_names(&self) -> Vec<&String> {
        self.symbols.keys().collect()
    }
    
    pub fn is_function_scope(&self) -> bool {
        matches!(self.scope_type, ScopeType::Function)
    }
    
    pub fn is_global_scope(&self) -> bool {
        matches!(self.scope_type, ScopeType::Global)
    }
    
    pub fn is_loop_scope(&self) -> bool {
        matches!(self.scope_type, ScopeType::Loop)
    }
}

/// Symbol table yang thread-safe
#[derive(Debug)]
pub struct SymbolTable {
    scopes: Arc<RwLock<HashMap<ScopeId, Scope>>>,
    current_scope: Arc<RwLock<ScopeId>>,
    next_scope_id: Arc<RwLock<usize>>,
}

impl SymbolTable {
    pub fn new() -> Self {
        let mut scopes = HashMap::new();
        let global_scope = Scope::new(ScopeId::global(), ScopeType::Global, None);
        scopes.insert(ScopeId::global(), global_scope);
        
        SymbolTable {
            scopes: Arc::new(RwLock::new(scopes)),
            current_scope: Arc::new(RwLock::new(ScopeId::global())),
            next_scope_id: Arc::new(RwLock::new(1)),
        }
    }
    
    /// Push scope baru ke stack
    pub fn push_scope(&self, scope_type: ScopeType) -> ScopeId {
        let mut next_id = self.next_scope_id.write();
        let scope_id = ScopeId::new(*next_id);
        *next_id += 1;
        
        let current_scope_id = *self.current_scope.read();
        
        let new_scope = Scope::new(scope_id, scope_type, Some(current_scope_id));
        
        // Add as child to parent scope
        {
            let mut scopes = self.scopes.write();
            if let Some(parent) = scopes.get_mut(&current_scope_id) {
                parent.children.push(scope_id);
            }
        }
        
        // Insert new scope
        {
            let mut scopes = self.scopes.write();
            scopes.insert(scope_id, new_scope);
        }
        
        // Update current scope
        *self.current_scope.write() = scope_id;
        
        scope_id
    }
    
    /// Pop scope saat ini dan return ke parent
    pub fn pop_scope(&self) -> Option<ScopeId> {
        let current_scope_id = *self.current_scope.read();
        
        let parent_scope_id = {
            let scopes = self.scopes.read();
            if let Some(current) = scopes.get(&current_scope_id) {
                current.parent
            } else {
                return None;
            }
        };
        
        if let Some(parent) = parent_scope_id {
            *self.current_scope.write() = parent;
            Some(current_scope_id)
        } else {
            None
        }
    }

    /// Mendapatkan level scope saat ini (0 untuk global)
    pub fn current_scope_level(&self) -> usize {
        let current_scope_id = *self.current_scope.read();
        let scopes = self.scopes.read();
        
        let mut level = 0;
        let mut curr_id = current_scope_id;
        
        while let Some(scope) = scopes.get(&curr_id) {
            if let Some(parent) = scope.parent {
                level += 1;
                curr_id = parent;
            } else {
                break;
            }
        }
        level
    }
    
    /// Insert simbol ke scope saat ini
    pub fn insert(&self, name: String, symbol: SymbolInfo) -> Result<()> {
        let current_scope_id = *self.current_scope.read();
        
        let mut scopes = self.scopes.write();
        if let Some(scope) = scopes.get_mut(&current_scope_id) {
            // Check for redeclaration in current scope
            if scope.lookup_current(&name).is_some() {
                return Err(SemanticError::Redeclaration { name });
            }
            
            scope.insert(name, symbol);
            Ok(())
        } else {
            Err(SemanticError::GeneralError {
                message: format!("Current scope {} not found", current_scope_id.as_usize()),
            })
        }
    }
    
    /// Lookup simbol hanya di scope saat ini
    pub fn lookup_current_scope(&self, name: &str) -> Option<SymbolInfo> {
        let current_scope_id = *self.current_scope.read();
        let scopes = self.scopes.read();
        
        if let Some(scope) = scopes.get(&current_scope_id) {
            scope.lookup_current(name).cloned()
        } else {
            None
        }
    }
    
    /// Lookup simbol dari scope saat ini ke atas
    pub fn lookup(&self, name: &str) -> Option<SymbolInfo> {
        let current_scope_id = *self.current_scope.read();
        
        let scopes = self.scopes.read();
        let mut scope_id = current_scope_id;
        
        loop {
            if let Some(scope) = scopes.get(&scope_id) {
                if let Some(symbol) = scope.lookup(name) {
                    return Some(symbol.clone());
                }
                
                if let Some(parent) = scope.parent {
                    scope_id = parent;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        
        None
    }
    
    /// Lookup simbol di scope tertentu
    pub fn lookup_in_scope(&self, name: &str, scope_id: ScopeId) -> Option<SymbolInfo> {
        let scopes = self.scopes.read();
        
        let mut current_id = scope_id;
        loop {
            if let Some(scope) = scopes.get(&current_id) {
                if let Some(symbol) = scope.lookup(name) {
                    return Some(symbol.clone());
                }
                
                if let Some(parent) = scope.parent {
                    current_id = parent;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        
        None
    }
    
    /// Remove simbol dari scope saat ini
    pub fn remove(&self, name: &str) -> Result<Option<SymbolInfo>> {
        let current_scope_id = *self.current_scope.read();
        
        let mut scopes = self.scopes.write();
        if let Some(scope) = scopes.get_mut(&current_scope_id) {
            Ok(scope.remove(name))
        } else {
            Err(SemanticError::GeneralError {
                message: format!("Current scope {} not found", current_scope_id.as_usize()),
            })
        }
    }
    
    /// Get scope saat ini
    pub fn current_scope(&self) -> ScopeId {
        *self.current_scope.read()
    }
    
    /// Get semua simbol di scope saat ini
    pub fn get_current_scope_symbols(&self) -> HashMap<String, SymbolInfo> {
        let current_scope_id = *self.current_scope.read();
        
        let scopes = self.scopes.read();
        if let Some(scope) = scopes.get(&current_scope_id) {
            scope.get_symbols().clone()
        } else {
            HashMap::new()
        }
    }
    
    /// Get scope by ID
    pub fn get_scope(&self, scope_id: ScopeId) -> Option<Scope> {
        let scopes = self.scopes.read();
        scopes.get(&scope_id).cloned()
    }
    
    /// Get all scopes
    pub fn get_all_scopes(&self) -> Vec<Scope> {
        let scopes = self.scopes.read();
        scopes.values().cloned().collect()
    }
    
    /// Clear semua simbol dari scope saat ini
    pub fn clear_current_scope(&self) {
        let current_scope_id = *self.current_scope.read();
        
        let mut scopes = self.scopes.write();
        if let Some(scope) = scopes.get_mut(&current_scope_id) {
            scope.symbols.clear();
        }
    }
    
    /// Check if current scope is of specific type
    pub fn current_scope_is(&self, scope_type: ScopeType) -> bool {
        let current_scope_id = *self.current_scope.read();
        
        let scopes = self.scopes.read();
        if let Some(scope) = scopes.get(&current_scope_id) {
            scope.scope_type == scope_type
        } else {
            false
        }
    }
    
    /// Find scope by type, searching up the hierarchy
    pub fn find_scope(&self, scope_type: ScopeType) -> Option<ScopeId> {
        let current_scope_id = *self.current_scope.read();
        
        let scopes = self.scopes.read();
        let mut current_id = current_scope_id;
        
        loop {
            if let Some(scope) = scopes.get(&current_id) {
                if scope.scope_type == scope_type {
                    return Some(current_id);
                }
                
                if let Some(parent) = scope.parent {
                    current_id = parent;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        
        None
    }
}

impl Clone for SymbolTable {
    fn clone(&self) -> Self {
        SymbolTable {
            scopes: Arc::new(RwLock::new(self.scopes.read().clone())),
            current_scope: Arc::new(RwLock::new(*self.current_scope.read())),
            next_scope_id: Arc::new(RwLock::new(*self.next_scope_id.read())),
        }
    }
}

impl Default for SymbolTable {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_symbol_table_creation() {
        let table = SymbolTable::new();
        assert_eq!(table.current_scope(), ScopeId::global());
        assert_eq!(table.current_scope_level(), 0);
    }
    
    #[test]
    fn test_scope_push_pop() {
        let table = SymbolTable::new();
        
        let scope1 = table.push_scope(ScopeType::Function);
        assert_eq!(table.current_scope(), scope1);
        assert_eq!(table.current_scope_level(), 1);
        
        let scope2 = table.push_scope(ScopeType::Block);
        assert_eq!(table.current_scope(), scope2);
        assert_eq!(table.current_scope_level(), 2);
        
        let popped = table.pop_scope();
        assert_eq!(popped, Some(scope2));
        assert_eq!(table.current_scope(), scope1);
        assert_eq!(table.current_scope_level(), 1);
    }
    
    #[test]
    fn test_symbol_insertion_and_lookup() {
        let table = SymbolTable::new();
        
        let symbol = SymbolInfo {
            name: "test_var".to_string(),
            symbol_type: SymbolType::Variable,
            type_info: crate::TypeInfo {
                type_kind: crate::TypeKind::Integer { bits: 32 },
                size: 4,
                alignment: 4,
                is_const: false,
                is_volatile: false,
                is_signed: true,
            },
            scope_level: 1,
            is_mutable: true,
            is_defined: true,
            location: None,
            attributes: HashMap::new(),
        };
        
        let func_scope = table.push_scope(ScopeType::Function);
        table.insert("test_var".to_string(), symbol.clone()).unwrap();
        
        let found = table.lookup("test_var");
        assert!(found.is_some());
        assert_eq!(found.unwrap().name, "test_var");
    }
    
    #[test]
    fn test_symbol_shadowing() {
        let table = SymbolTable::new();
        
        let outer_symbol = SymbolInfo {
            name: "x".to_string(),
            symbol_type: SymbolType::Variable,
            type_info: crate::TypeInfo {
                type_kind: crate::TypeKind::Integer { bits: 32 },
                size: 4,
                alignment: 4,
                is_const: false,
                is_volatile: false,
                is_signed: true,
            },
            scope_level: 1,
            is_mutable: true,
            is_defined: true,
            location: None,
            attributes: HashMap::new(),
        };
        
        let inner_symbol = SymbolInfo {
            name: "x".to_string(),
            symbol_type: SymbolType::Variable,
            type_info: crate::TypeInfo {
                type_kind: crate::TypeKind::Float { bits: 64 },
                size: 8,
                alignment: 8,
                is_const: false,
                is_volatile: false,
                is_signed: true,
            },
            scope_level: 2,
            is_mutable: true,
            is_defined: true,
            location: None,
            attributes: HashMap::new(),
        };
        
        let outer_scope = table.push_scope(ScopeType::Function);
        table.insert("x".to_string(), outer_symbol).unwrap();
        
        let inner_scope = table.push_scope(ScopeType::Block);
        table.insert("x".to_string(), inner_symbol.clone()).unwrap();
        
        // Should find inner symbol
        let found = table.lookup("x");
        assert!(found.is_some());
        let symbol = found.unwrap();
        assert_eq!(symbol.name, "x");
        // Should be the float type (inner symbol)
        match &symbol.type_info.type_kind {
            crate::TypeKind::Float { bits: 64 } => {},
            _ => panic!("Expected float type"),
        }
    }
    
    #[test]
    fn test_redeclaration_error() {
        let table = SymbolTable::new();
        
        let symbol = SymbolInfo {
            name: "test".to_string(),
            symbol_type: SymbolType::Variable,
            type_info: crate::TypeInfo {
                type_kind: crate::TypeKind::Integer { bits: 32 },
                size: 4,
                alignment: 4,
                is_const: false,
                is_volatile: false,
                is_signed: true,
            },
            scope_level: 1,
            is_mutable: true,
            is_defined: true,
            location: None,
            attributes: HashMap::new(),
        };
        
        let scope = table.push_scope(ScopeType::Function);
        table.insert("test".to_string(), symbol.clone()).unwrap();
        
        // Should fail with redeclaration error
        let result = table.insert("test".to_string(), symbol);
        assert!(result.is_err());
        match result.unwrap_err() {
            SemanticError::Redeclaration { name } => assert_eq!(name, "test"),
            _ => panic!("Expected redeclaration error"),
        }
    }
}