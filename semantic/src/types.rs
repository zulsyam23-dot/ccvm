//! Type system untuk semantic analysis

use std::collections::HashMap;
use crate::{SemanticError, Result, SourceLocation};
use serde::{Serialize, Deserialize};

/// Informasi tentang tipe data
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct TypeInfo {
    pub type_kind: TypeKind,
    pub size: usize,
    pub alignment: usize,
    pub is_const: bool,
    pub is_volatile: bool,
    pub is_signed: bool,
}

/// Jenis-jenis tipe data
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub enum TypeKind {
    Void,
    Bool,
    Integer { bits: u32 },
    Float { bits: u32 },
    Pointer(Box<TypeInfo>),
    Array { element: Box<TypeInfo>, size: usize },
    Function { return_type: Box<TypeInfo>, param_types: Vec<TypeInfo>, is_variadic: bool },
    Struct { name: String, fields: Vec<(String, TypeInfo)> },
    Union { name: String, fields: Vec<(String, TypeInfo)> },
    Enum { name: String, variants: Vec<String> },
    Typedef { name: String, underlying: Box<TypeInfo> },
    Generic { name: String, constraints: Vec<String> },
    Alias(String),
    Named(String),
    Unknown,
}

/// Informasi tentang simbol
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct SymbolInfo {
    pub name: String,
    pub symbol_type: SymbolType,
    pub type_info: TypeInfo,
    pub scope_level: usize,
    pub is_mutable: bool,
    pub is_defined: bool,
    pub location: Option<SourceLocation>,
    pub attributes: HashMap<String, String>,
}

/// Jenis-jenis simbol
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub enum SymbolType {
    Variable,
    Function,
    Constant,
    Type,
    Namespace,
}

/// Type system yang komprehensif
#[derive(Debug, Clone)]
pub struct TypeSystem {
    /// Mapping dari nama tipe ke definisi
    type_definitions: HashMap<String, TypeInfo>,
    
    /// Built-in types
    builtin_types: HashMap<String, TypeInfo>,
    
    /// User-defined types
    user_types: HashMap<String, TypeInfo>,
    
    /// Type aliases
    type_aliases: HashMap<String, String>,
    
    /// Generic type parameters
    generic_params: HashMap<String, Vec<String>>,
}

impl TypeSystem {
    pub fn new() -> Self {
        let mut type_system = TypeSystem {
            type_definitions: HashMap::new(),
            builtin_types: HashMap::new(),
            user_types: HashMap::new(),
            type_aliases: HashMap::new(),
            generic_params: HashMap::new(),
        };
        
        type_system.initialize_builtin_types();
        type_system
    }
    
    fn initialize_builtin_types(&mut self) {
        // Primitive integer types
        self.builtin_types.insert("i8".to_string(), TypeInfo {
            type_kind: TypeKind::Integer { bits: 8 },
            size: 1,
            alignment: 1,
            is_const: false,
            is_volatile: false,
            is_signed: true,
        });
        
        self.builtin_types.insert("i16".to_string(), TypeInfo {
            type_kind: TypeKind::Integer { bits: 16 },
            size: 2,
            alignment: 2,
            is_const: false,
            is_volatile: false,
            is_signed: true,
        });
        
        self.builtin_types.insert("i32".to_string(), TypeInfo {
            type_kind: TypeKind::Integer { bits: 32 },
            size: 4,
            alignment: 4,
            is_const: false,
            is_volatile: false,
            is_signed: true,
        });
        
        self.builtin_types.insert("i64".to_string(), TypeInfo {
            type_kind: TypeKind::Integer { bits: 64 },
            size: 8,
            alignment: 8,
            is_const: false,
            is_volatile: false,
            is_signed: true,
        });
        
        // Primitive unsigned integer types
        self.builtin_types.insert("u8".to_string(), TypeInfo {
            type_kind: TypeKind::Integer { bits: 8 },
            size: 1,
            alignment: 1,
            is_const: false,
            is_volatile: false,
            is_signed: false,
        });
        
        self.builtin_types.insert("u16".to_string(), TypeInfo {
            type_kind: TypeKind::Integer { bits: 16 },
            size: 2,
            alignment: 2,
            is_const: false,
            is_volatile: false,
            is_signed: false,
        });
        
        self.builtin_types.insert("u32".to_string(), TypeInfo {
            type_kind: TypeKind::Integer { bits: 32 },
            size: 4,
            alignment: 4,
            is_const: false,
            is_volatile: false,
            is_signed: false,
        });
        
        self.builtin_types.insert("u64".to_string(), TypeInfo {
            type_kind: TypeKind::Integer { bits: 64 },
            size: 8,
            alignment: 8,
            is_const: false,
            is_volatile: false,
            is_signed: false,
        });
        
        // Floating point types
        self.builtin_types.insert("f32".to_string(), TypeInfo {
            type_kind: TypeKind::Float { bits: 32 },
            size: 4,
            alignment: 4,
            is_const: false,
            is_volatile: false,
            is_signed: true,
        });
        
        self.builtin_types.insert("f64".to_string(), TypeInfo {
            type_kind: TypeKind::Float { bits: 64 },
            size: 8,
            alignment: 8,
            is_const: false,
            is_volatile: false,
            is_signed: true,
        });
        
        // Other primitive types
        self.builtin_types.insert("bool".to_string(), TypeInfo {
            type_kind: TypeKind::Bool,
            size: 1,
            alignment: 1,
            is_const: false,
            is_volatile: false,
            is_signed: false,
        });
        
        self.builtin_types.insert("void".to_string(), TypeInfo {
            type_kind: TypeKind::Void,
            size: 0,
            alignment: 0,
            is_const: false,
            is_volatile: false,
            is_signed: false,
        });
    }
    
    /// Register built-in type
    pub fn register_builtin_type(&mut self, name: &str, type_info: TypeInfo) {
        self.builtin_types.insert(name.to_string(), type_info);
    }
    
    /// Register user-defined type
    pub fn register_user_type(&mut self, name: &str, type_info: TypeInfo) -> Result<()> {
        if self.builtin_types.contains_key(name) {
            return Err(SemanticError::GeneralError {
                message: format!("Cannot redefine built-in type '{}'", name),
            });
        }
        
        if self.user_types.contains_key(name) {
            return Err(SemanticError::GeneralError {
                message: format!("Type '{}' already defined", name),
            });
        }
        
        self.user_types.insert(name.to_string(), type_info.clone());
        self.type_definitions.insert(name.to_string(), type_info);
        
        Ok(())
    }
    
    /// Create type alias
    pub fn create_type_alias(&mut self, alias: &str, original: &str) -> Result<()> {
        if self.type_aliases.contains_key(alias) {
            return Err(SemanticError::GeneralError {
                message: format!("Type alias '{}' already exists", alias),
            });
        }
        
        // Check if original type exists
        if !self.type_exists(original) {
            return Err(SemanticError::GeneralError {
                message: format!("Original type '{}' not found", original),
            });
        }
        
        self.type_aliases.insert(alias.to_string(), original.to_string());
        Ok(())
    }
    
    /// Get type info by name
    pub fn get_type(&self, name: &str) -> Option<TypeInfo> {
        // Check aliases first
        if let Some(original) = self.type_aliases.get(name) {
            return self.get_type(original);
        }
        
        // Check built-in types
        if let Some(type_info) = self.builtin_types.get(name) {
            return Some(type_info.clone());
        }
        
        // Check user-defined types
        if let Some(type_info) = self.user_types.get(name) {
            return Some(type_info.clone());
        }
        
        None
    }
    
    /// Check if type exists
    pub fn type_exists(&self, name: &str) -> bool {
        self.builtin_types.contains_key(name) || 
        self.user_types.contains_key(name) ||
        self.type_aliases.contains_key(name)
    }
    
    /// Check type compatibility
    pub fn are_compatible(&self, type1: &TypeInfo, type2: &TypeInfo) -> bool {
        // Exact match
        if type1 == type2 {
            return true;
        }
        
        // Const/volatile compatibility
        if self.is_const_compatible(type1, type2) {
            return true;
        }
        
        // Numeric type promotions
        if self.is_numeric_promotion_compatible(type1, type2) {
            return true;
        }
        
        // Pointer compatibility
        if self.is_pointer_compatible(type1, type2) {
            return true;
        }
        
        // Function compatibility
        if self.is_function_compatible(type1, type2) {
            return true;
        }
        
        false
    }
    
    /// Check if types are const compatible
    fn is_const_compatible(&self, type1: &TypeInfo, type2: &TypeInfo) -> bool {
        // Remove const/volatile qualifiers and compare
        let mut t1 = type1.clone();
        let mut t2 = type2.clone();
        
        t1.is_const = false;
        t1.is_volatile = false;
        t2.is_const = false;
        t2.is_volatile = false;
        
        t1 == t2
    }
    
    /// Check numeric type promotions
    fn is_numeric_promotion_compatible(&self, type1: &TypeInfo, type2: &TypeInfo) -> bool {
        use TypeKind::*;
        
        match (&type1.type_kind, &type2.type_kind) {
            (Integer { .. }, Integer { .. }) => {
                // All integers are compatible for now (allow narrowing/widening)
                true
            }
            
            (Float { .. }, Float { .. }) => {
                // All floats are compatible for now
                true
            }
            
            (Float { .. }, Integer { .. }) | (Integer { .. }, Float { .. }) => {
                // Integer <-> Float conversion allowed
                true
            }
            
            _ => false,
        }
    }
    
    /// Check pointer compatibility
    fn is_pointer_compatible(&self, type1: &TypeInfo, type2: &TypeInfo) -> bool {
        use TypeKind::*;
        
        match (&type1.type_kind, &type2.type_kind) {
            (Pointer(ptr1), Pointer(ptr2)) => {
                // Void pointer is compatible with any pointer
                if matches!(ptr1.type_kind, TypeKind::Void) || 
                   matches!(ptr2.type_kind, TypeKind::Void) {
                    return true;
                }
                
                // Same pointer type
                self.are_compatible(ptr1, ptr2)
            }
            
            _ => false,
        }
    }
    
    /// Check function compatibility
    fn is_function_compatible(&self, type1: &TypeInfo, type2: &TypeInfo) -> bool {
        use TypeKind::*;
        
        match (&type1.type_kind, &type2.type_kind) {
            (Function { return_type: ret1, param_types: params1, is_variadic: var1 },
             Function { return_type: ret2, param_types: params2, is_variadic: var2 }) => {
                
                // Return types must be compatible
                if !self.are_compatible(ret1, ret2) {
                    return false;
                }
                
                // Parameter count must match (unless variadic)
                if params1.len() != params2.len() && !var1 && !var2 {
                    return false;
                }
                
                // Parameter types must be compatible
                for (p1, p2) in params1.iter().zip(params2.iter()) {
                    if !self.are_compatible(p1, p2) {
                        return false;
                    }
                }
                
                // Variadic compatibility
                if *var1 && !var2 {
                    return false; // Cannot convert non-variadic to variadic
                }
                
                true
            }
            
            _ => false,
        }
    }
    
    /// Promote types for binary operations
    pub fn promote_for_binary_op(&self, type1: &TypeInfo, type2: &TypeInfo) -> Option<TypeInfo> {
        use TypeKind::*;
        
        match (&type1.type_kind, &type2.type_kind) {
            // Both integers
            (Integer { bits: bits1 }, Integer { bits: bits2 }) => {
                let max_bits = bits1.max(bits2);
                
                // If either is signed, result is signed
                let is_signed = type1.is_signed || type2.is_signed;
                
                Some(TypeInfo {
                    type_kind: Integer { bits: *max_bits },
                    size: (*max_bits / 8) as usize,
                    alignment: (*max_bits / 8) as usize,
                    is_const: false,
                    is_volatile: false,
                    is_signed,
                })
            }
            
            // Both floats
            (Float { bits: bits1 }, Float { bits: bits2 }) => {
                let max_bits = bits1.max(bits2);
                
                Some(TypeInfo {
                    type_kind: Float { bits: *max_bits },
                    size: (*max_bits / 8) as usize,
                    alignment: (*max_bits / 8) as usize,
                    is_const: false,
                    is_volatile: false,
                    is_signed: true,
                })
            }
            
            // Integer and float
            (Integer { .. }, Float { bits }) |
            (Float { bits }, Integer { .. }) => {
                Some(TypeInfo {
                    type_kind: Float { bits: *bits },
                    size: (bits / 8) as usize,
                    alignment: (bits / 8) as usize,
                    is_const: false,
                    is_volatile: false,
                    is_signed: true,
                })
            }
            
            _ => None,
        }
    }
    
    /// Get common type for conditional expressions
    pub fn common_type(&self, type1: &TypeInfo, type2: &TypeInfo) -> Option<TypeInfo> {
        // If types are the same, return either
        if type1 == type2 {
            return Some(type1.clone());
        }
        
        // Try promotion for binary operations
        self.promote_for_binary_op(type1, type2)
    }
    
    /// Check if type is complete (fully defined)
    pub fn is_complete(&self, type_info: &TypeInfo) -> bool {
        use TypeKind::*;
        
        match &type_info.type_kind {
            Void => false,
            Bool | Integer { .. } | Float { .. } => true,
            Pointer(_) => true, // Pointers are always complete
            Array { element, size } => *size > 0 && self.is_complete(element),
            Function { .. } => true,
            Struct { fields, .. } => !fields.is_empty(),
            Union { fields, .. } => !fields.is_empty(),
            Enum { variants, .. } => !variants.is_empty(),
            Typedef { underlying, .. } => self.is_complete(underlying),
            Alias(_) | Named(_) | Unknown => false,
            Generic { .. } => false, // Generic types are incomplete until instantiated
        }
    }
    
    /// Get size of type
    pub fn get_size(&self, type_info: &TypeInfo) -> usize {
        type_info.size
    }
    
    /// Get alignment of type
    pub fn get_alignment(&self, type_info: &TypeInfo) -> usize {
        type_info.alignment
    }
    
    /// Check if type is scalar
    pub fn is_scalar(&self, type_info: &TypeInfo) -> bool {
        use TypeKind::*;
        
        matches!(&type_info.type_kind,
            Bool | Integer { .. } | Float { .. } | Pointer(_) | Enum { .. }
        )
    }
    
    /// Check if type is aggregate
    pub fn is_aggregate(&self, type_info: &TypeInfo) -> bool {
        use TypeKind::*;
        
        matches!(&type_info.type_kind,
            Array { .. } | Struct { .. } | Union { .. }
        )
    }
    
    /// Check if type is arithmetic
    pub fn is_arithmetic(&self, type_info: &TypeInfo) -> bool {
        use TypeKind::*;
        
        matches!(&type_info.type_kind,
            Bool | Integer { .. } | Float { .. }
        )
    }
    
    /// Check if type is integer
    pub fn is_integer(&self, type_info: &TypeInfo) -> bool {
        use TypeKind::*;
        
        matches!(&type_info.type_kind, Integer { .. })
    }
    
    /// Check if type is floating point
    pub fn is_float(&self, type_info: &TypeInfo) -> bool {
        use TypeKind::*;
        
        matches!(&type_info.type_kind, Float { .. })
    }
    
    /// Check if type is pointer
    pub fn is_pointer(&self, type_info: &TypeInfo) -> bool {
        use TypeKind::*;
        
        matches!(&type_info.type_kind, Pointer(_))
    }
    
    /// Check if type is function
    pub fn is_function(&self, type_info: &TypeInfo) -> bool {
        use TypeKind::*;
        
        matches!(&type_info.type_kind, Function { .. })
    }
    
    /// Check if type is void
    pub fn is_void(&self, type_info: &TypeInfo) -> bool {
        use TypeKind::*;
        
        matches!(&type_info.type_kind, Void)
    }
    
    /// Check if type is const
    pub fn is_const(&self, type_info: &TypeInfo) -> bool {
        type_info.is_const
    }
    
    /// Check if type is volatile
    pub fn is_volatile(&self, type_info: &TypeInfo) -> bool {
        type_info.is_volatile
    }
    
    /// Add const qualifier
    pub fn add_const(&self, type_info: &TypeInfo) -> TypeInfo {
        let mut result = type_info.clone();
        result.is_const = true;
        result
    }
    
    /// Remove const qualifier
    pub fn remove_const(&self, type_info: &TypeInfo) -> TypeInfo {
        let mut result = type_info.clone();
        result.is_const = false;
        result
    }
    
    /// Add volatile qualifier
    pub fn add_volatile(&self, type_info: &TypeInfo) -> TypeInfo {
        let mut result = type_info.clone();
        result.is_volatile = true;
        result
    }
    
    /// Remove volatile qualifier
    pub fn remove_volatile(&self, type_info: &TypeInfo) -> TypeInfo {
        let mut result = type_info.clone();
        result.is_volatile = false;
        result
    }
}

impl Default for TypeSystem {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_type_system_creation() {
        let type_system = TypeSystem::new();
        
        // Check built-in types exist
        assert!(type_system.type_exists("i32"));
        assert!(type_system.type_exists("f64"));
        assert!(type_system.type_exists("bool"));
        assert!(type_system.type_exists("void"));
    }
    
    #[test]
    fn test_type_compatibility() {
        let type_system = TypeSystem::new();
        
        let i32_type = type_system.get_type("i32").unwrap();
        let i64_type = type_system.get_type("i64").unwrap();
        let f32_type = type_system.get_type("f32").unwrap();
        let f64_type = type_system.get_type("f64").unwrap();
        
        // Same types should be compatible
        assert!(type_system.are_compatible(&i32_type, &i32_type));
        
        // Integer promotion should work
        assert!(type_system.are_compatible(&i32_type, &i64_type));
        assert!(type_system.are_compatible(&i64_type, &i32_type));
        
        // Float promotion should work
        assert!(type_system.are_compatible(&f32_type, &f64_type));
        assert!(type_system.are_compatible(&f64_type, &f32_type));
        
        // Integer to float should work
        assert!(type_system.are_compatible(&i32_type, &f64_type));
        assert!(type_system.are_compatible(&f64_type, &i32_type));
    }
    
    #[test]
    fn test_type_promotion() {
        let type_system = TypeSystem::new();
        
        let i32_type = type_system.get_type("i32").unwrap();
        let i64_type = type_system.get_type("i64").unwrap();
        let f32_type = type_system.get_type("f32").unwrap();
        let f64_type = type_system.get_type("f64").unwrap();
        
        // Integer promotion
        let promoted = type_system.promote_for_binary_op(&i32_type, &i64_type);
        assert!(promoted.is_some());
        let promoted_type = promoted.unwrap();
        assert!(type_system.is_integer(&promoted_type));
        assert_eq!(promoted_type.size, 8); // i64
        
        // Float promotion
        let promoted = type_system.promote_for_binary_op(&f32_type, &f64_type);
        assert!(promoted.is_some());
        let promoted_type = promoted.unwrap();
        assert!(type_system.is_float(&promoted_type));
        assert_eq!(promoted_type.size, 8); // f64
        
        // Mixed promotion
        let promoted = type_system.promote_for_binary_op(&i32_type, &f64_type);
        assert!(promoted.is_some());
        let promoted_type = promoted.unwrap();
        assert!(type_system.is_float(&promoted_type));
        assert_eq!(promoted_type.size, 8); // f64
    }
    
    #[test]
    fn test_type_aliases() {
        let mut type_system = TypeSystem::new();
        
        // Create alias
        type_system.create_type_alias("my_int", "i32").unwrap();
        
        // Should be able to get type through alias
        let original = type_system.get_type("i32").unwrap();
        let aliased = type_system.get_type("my_int").unwrap();
        
        assert_eq!(original, aliased);
        
        // Both should exist
        assert!(type_system.type_exists("i32"));
        assert!(type_system.type_exists("my_int"));
    }
    
    #[test]
    fn test_const_qualifier() {
        let type_system = TypeSystem::new();
        let i32_type = type_system.get_type("i32").unwrap();
        
        // Add const
        let const_i32 = type_system.add_const(&i32_type);
        assert!(type_system.is_const(&const_i32));
        assert!(!type_system.is_const(&i32_type));
        
        // Remove const
        let non_const = type_system.remove_const(&const_i32);
        assert!(!type_system.is_const(&non_const));
        assert_eq!(non_const, i32_type);
    }
    
    #[test]
    fn test_type_properties() {
        let type_system = TypeSystem::new();
        
        let i32_type = type_system.get_type("i32").unwrap();
        let f64_type = type_system.get_type("f64").unwrap();
        let bool_type = type_system.get_type("bool").unwrap();
        
        // Integer properties
        assert!(type_system.is_integer(&i32_type));
        assert!(!type_system.is_float(&i32_type));
        assert!(type_system.is_scalar(&i32_type));
        assert!(type_system.is_arithmetic(&i32_type));
        
        // Float properties
        assert!(!type_system.is_integer(&f64_type));
        assert!(type_system.is_float(&f64_type));
        assert!(type_system.is_scalar(&f64_type));
        assert!(type_system.is_arithmetic(&f64_type));
        
        // Bool properties
        assert!(!type_system.is_integer(&bool_type));
        assert!(!type_system.is_float(&bool_type));
        assert!(type_system.is_scalar(&bool_type));
        assert!(type_system.is_arithmetic(&bool_type));
    }
}