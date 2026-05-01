//! Type system untuk CCVM IR

use serde::{Serialize, Deserialize};
use std::fmt;
use crate::{CoreError, Result};

/// Primitive types dalam CCVM
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum PrimitiveType {
    I1,     // 1-bit integer (boolean)
    I8,     // 8-bit integer
    I16,    // 16-bit integer
    I32,    // 32-bit integer
    I64,    // 64-bit integer
    I128,   // 128-bit integer
    F16,    // 16-bit floating point
    F32,    // 32-bit floating point
    F64,    // 64-bit floating point
    F128,   // 128-bit floating point
}

impl PrimitiveType {
    /// Ukuran dalam bits
    pub fn bit_width(&self) -> u32 {
        match self {
            PrimitiveType::I1 => 1,
            PrimitiveType::I8 => 8,
            PrimitiveType::I16 => 16,
            PrimitiveType::I32 => 32,
            PrimitiveType::I64 => 64,
            PrimitiveType::I128 => 128,
            PrimitiveType::F16 => 16,
            PrimitiveType::F32 => 32,
            PrimitiveType::F64 => 64,
            PrimitiveType::F128 => 128,
        }
    }

    /// Ukuran dalam bytes
    pub fn byte_size(&self) -> u32 {
        (self.bit_width() + 7) / 8
    }

    /// Apakah tipe ini floating point?
    pub fn is_float(&self) -> bool {
        matches!(self, PrimitiveType::F16 | PrimitiveType::F32 | PrimitiveType::F64 | PrimitiveType::F128)
    }

    /// Apakah tipe ini integer?
    pub fn is_integer(&self) -> bool {
        !self.is_float()
    }

    /// Apakah tipe ini signed?
    pub fn is_signed(&self) -> bool {
        !matches!(self, PrimitiveType::I1)
    }
}

impl fmt::Display for PrimitiveType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            PrimitiveType::I1 => write!(f, "i1"),
            PrimitiveType::I8 => write!(f, "i8"),
            PrimitiveType::I16 => write!(f, "i16"),
            PrimitiveType::I32 => write!(f, "i32"),
            PrimitiveType::I64 => write!(f, "i64"),
            PrimitiveType::I128 => write!(f, "i128"),
            PrimitiveType::F16 => write!(f, "f16"),
            PrimitiveType::F32 => write!(f, "f32"),
            PrimitiveType::F64 => write!(f, "f64"),
            PrimitiveType::F128 => write!(f, "f128"),
        }
    }
}

/// Vector types untuk SIMD
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct VectorType {
    pub element_type: Box<Type>,
    pub num_elements: u32,
}

/// Array types
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct ArrayType {
    pub element_type: Box<Type>,
    pub num_elements: u32,
}

/// Pointer types
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct PointerType {
    pub pointee_type: Box<Type>,
    pub address_space: u32,
}

/// Function types
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct FunctionType {
    pub return_type: Box<Type>,
    pub parameter_types: Vec<Type>,
    pub is_vararg: bool,
}

/// Struct types
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct StructType {
    pub name: Option<String>,
    pub fields: Vec<(String, Type)>,
    pub is_packed: bool,
}

/// Semua tipe dalam CCVM
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum Type {
    Primitive(PrimitiveType),
    Vector(VectorType),
    Array(ArrayType),
    Pointer(PointerType),
    Function(FunctionType),
    Struct(StructType),
    Void,
    Label,
    Metadata,
}

impl Type {
    /// Ukuran dalam bits
    pub fn bit_width(&self) -> u32 {
        match self {
            Type::Primitive(p) => p.bit_width(),
            Type::Vector(v) => v.element_type.bit_width() * v.num_elements,
            Type::Array(a) => a.element_type.bit_width() * a.num_elements,
            Type::Pointer(_) => 64, // Assume 64-bit architecture
            Type::Struct(s) => {
                s.fields.iter()
                    .map(|(_, ty)| ty.bit_width())
                    .sum()
            },
            Type::Function(_) => 0,
            Type::Void => 0,
            Type::Label => 0,
            Type::Metadata => 0,
        }
    }

    /// Ukuran dalam bytes
    pub fn byte_size(&self) -> u32 {
        (self.bit_width() + 7) / 8
    }

    /// Alignment requirement
    pub fn alignment(&self) -> u32 {
        match self {
            Type::Primitive(p) => p.byte_size(),
            Type::Vector(v) => v.element_type.alignment(),
            Type::Array(a) => a.element_type.alignment(),
            Type::Pointer(_) => 8,
            Type::Struct(s) => {
                s.fields.iter()
                    .map(|(_, ty)| ty.alignment())
                    .max()
                    .unwrap_or(1)
            },
            _ => 1,
        }
    }

    /// Apakah tipe ini primitive?
    pub fn is_primitive(&self) -> bool {
        matches!(self, Type::Primitive(_))
    }

    /// Apakah tipe ini integer?
    pub fn is_integer(&self) -> bool {
        match self {
            Type::Primitive(p) => p.is_integer(),
            _ => false,
        }
    }

    /// Apakah tipe ini floating point?
    pub fn is_float(&self) -> bool {
        match self {
            Type::Primitive(p) => p.is_float(),
            _ => false,
        }
    }

    /// Apakah tipe ini pointer?
    pub fn is_pointer(&self) -> bool {
        matches!(self, Type::Pointer(_))
    }

    /// Apakah tipe ini function?
    pub fn is_function(&self) -> bool {
        matches!(self, Type::Function(_))
    }

    /// Apakah tipe ini void?
    pub fn is_void(&self) -> bool {
        matches!(self, Type::Void)
    }

    /// Konversi ke primitive type
    pub fn as_primitive(&self) -> Option<&PrimitiveType> {
        match self {
            Type::Primitive(p) => Some(p),
            _ => None,
        }
    }

    /// Validasi tipe
    pub fn validate(&self) -> Result<()> {
        match self {
            Type::Vector(v) => {
                if v.num_elements == 0 {
                    return Err(CoreError::ValidationError("Vector cannot have 0 elements".to_string()));
                }
                v.element_type.validate()?;
            },
            Type::Array(a) => {
                if a.num_elements == 0 {
                    return Err(CoreError::ValidationError("Array cannot have 0 elements".to_string()));
                }
                a.element_type.validate()?;
            },
            Type::Pointer(p) => {
                if p.address_space > 255 {
                    return Err(CoreError::ValidationError("Invalid address space".to_string()));
                }
                p.pointee_type.validate()?;
            },
            Type::Function(f) => {
                f.return_type.validate()?;
                for param in &f.parameter_types {
                    param.validate()?;
                }
            },
            Type::Struct(s) => {
                if s.fields.is_empty() {
                    return Err(CoreError::ValidationError("Struct cannot have 0 fields".to_string()));
                }
                for (_, field_ty) in &s.fields {
                    field_ty.validate()?;
                }
            },
            _ => {}
        }
        Ok(())
    }
}

impl fmt::Display for Type {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Type::Primitive(p) => write!(f, "{}", p),
            Type::Vector(v) => write!(f, "vec<{} x {}>", v.element_type, v.num_elements),
            Type::Array(a) => write!(f, "[{} x {}]", a.element_type, a.num_elements),
            Type::Pointer(p) => write!(f, "ptr {}", p.pointee_type),
            Type::Function(func) => {
                write!(f, "func(")?;
                for (i, param) in func.parameter_types.iter().enumerate() {
                    if i > 0 { write!(f, ", ")?; }
                    write!(f, "{}", param)?;
                }
                if func.is_vararg {
                    if !func.parameter_types.is_empty() { write!(f, ", ")?; }
                    write!(f, "...")?;
                }
                write!(f, ") -> {}", func.return_type)
            },
            Type::Struct(s) => {
                if let Some(name) = &s.name {
                    write!(f, "%{}", name)
                } else {
                    write!(f, "struct {{ ")?;
                    for (i, (field_name, field_ty)) in s.fields.iter().enumerate() {
                        if i > 0 { write!(f, ", ")?; }
                        write!(f, "{}: {}", field_name, field_ty)?;
                    }
                    write!(f, " }}")
                }
            },
            Type::Void => write!(f, "void"),
            Type::Label => write!(f, "label"),
            Type::Metadata => write!(f, "metadata"),
        }
    }
}

/// Builder untuk membuat tipe
pub struct TypeBuilder {
    ty: Type,
}

impl TypeBuilder {
    pub fn primitive(p: PrimitiveType) -> Self {
        TypeBuilder {
            ty: Type::Primitive(p),
        }
    }

    pub fn void() -> Self {
        TypeBuilder {
            ty: Type::Void,
        }
    }

    pub fn pointer_to(pointee: Type) -> Self {
        TypeBuilder {
            ty: Type::Pointer(PointerType {
                pointee_type: Box::new(pointee),
                address_space: 0,
            }),
        }
    }

    pub fn array(element: Type, num_elements: u32) -> Self {
        TypeBuilder {
            ty: Type::Array(ArrayType {
                element_type: Box::new(element),
                num_elements,
            }),
        }
    }

    pub fn vector(element: Type, num_elements: u32) -> Self {
        TypeBuilder {
            ty: Type::Vector(VectorType {
                element_type: Box::new(element),
                num_elements,
            }),
        }
    }

    pub fn function(return_type: Type, parameter_types: Vec<Type>) -> Self {
        TypeBuilder {
            ty: Type::Function(FunctionType {
                return_type: Box::new(return_type),
                parameter_types,
                is_vararg: false,
            }),
        }
    }

    pub fn struct_type(fields: Vec<(String, Type)>) -> Self {
        TypeBuilder {
            ty: Type::Struct(StructType {
                name: None,
                fields,
                is_packed: false,
            }),
        }
    }

    pub fn build(self) -> Type {
        self.ty
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_primitive_types() {
        assert_eq!(PrimitiveType::I32.bit_width(), 32);
        assert_eq!(PrimitiveType::I64.byte_size(), 8);
        assert!(PrimitiveType::F64.is_float());
        assert!(PrimitiveType::I32.is_integer());
    }

    #[test]
    fn test_type_display() {
        let i32_ty = Type::Primitive(PrimitiveType::I32);
        assert_eq!(format!("{}", i32_ty), "i32");

        let ptr_ty = TypeBuilder::pointer_to(i32_ty.clone()).build();
        assert_eq!(format!("{}", ptr_ty), "ptr i32");

        let array_ty = TypeBuilder::array(i32_ty.clone(), 10).build();
        assert_eq!(format!("{}", array_ty), "[i32 x 10]");
    }

    #[test]
    fn test_type_validation() {
        let valid_ty = TypeBuilder::array(Type::Primitive(PrimitiveType::I32), 10).build();
        assert!(valid_ty.validate().is_ok());

        let invalid_ty = TypeBuilder::array(Type::Primitive(PrimitiveType::I32), 0).build();
        assert!(invalid_ty.validate().is_err());
    }
}