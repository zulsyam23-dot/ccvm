//! CCVM Core Library - IR dan Type System
//! 
//! Library ini menyediakan implementasi inti untuk CCVM IR termasuk:
//! - Type system yang komprehensif
//! - Instruction set architecture
//! - Module dan function representation
//! - Validation framework

use thiserror::Error;

pub mod types;
pub mod instruction;
pub mod module;
pub mod validation;
pub mod passes;

pub use types::*;
pub use instruction::*;
pub use module::*;

/// Error types untuk CCVM Core
#[derive(Error, Debug, Clone, PartialEq)]
pub enum CoreError {
    #[error("Type mismatch: expected {expected}, found {found}")]
    TypeMismatch { expected: String, found: String },
    
    #[error("Invalid instruction: {0}")]
    InvalidInstruction(String),
    
    #[error("Undefined symbol: {0}")]
    UndefinedSymbol(String),
    
    #[error("Validation error: {0}")]
    ValidationError(String),
    
    #[error("IO error: {0}")]
    IoError(String),
}

/// Hasil operasi CCVM Core
pub type Result<T> = std::result::Result<T, CoreError>;

/// Versi IR CCVM
pub const IR_VERSION: u32 = 1;

/// Magic header untuk format biner CCVM
pub const BINARY_MAGIC: &[u8] = b"CCVM\x01\x00\x00\x00";

/// Trait untuk entity yang memiliki identifier
pub trait Identifiable {
    fn id(&self) -> &str;
}

/// Trait untuk entity yang dapat divalidasi
pub trait Validatable {
    fn validate(&self) -> Result<()>;
}

/// Trait untuk entity yang memiliki tipe
pub trait Typed {
    fn get_type(&self) -> &Type;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_ir_version() {
        assert_eq!(IR_VERSION, 1);
    }

    #[test]
    fn test_binary_magic() {
        assert_eq!(BINARY_MAGIC, b"CCVM\x01\x00\x00\x00");
    }
}