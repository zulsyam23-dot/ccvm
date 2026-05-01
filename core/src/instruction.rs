//! Instruction system untuk CCVM IR

use serde::{Serialize, Deserialize};
use std::fmt;
use crate::{types::*, CoreError, Result};

/// Operand untuk instruction
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum Operand {
    /// Register SSA
    Register(String),
    /// Immediate value
    Immediate(Value),
    /// Global identifier
    Global(String),
    /// Basic block label
    Label(String),
}

impl fmt::Display for Operand {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Operand::Register(reg) => write!(f, "%{}", reg),
            Operand::Immediate(val) => write!(f, "{}", val),
            Operand::Global(name) => write!(f, "@{}", name),
            Operand::Label(label) => write!(f, "%{}", label),
        }
    }
}

/// Value types
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum Value {
    I1(bool),
    I8(i8),
    I16(i16),
    I32(i32),
    I64(i64),
    I128(i128),
    F16(u16),  // IEEE 754 half-precision bit representation
    F32(u32),  // IEEE 754 single-precision bit representation
    F64(u64),  // IEEE 754 double-precision bit representation
    F128(u128), // IEEE 754 quad-precision bit representation
    Array(Vec<Value>),
    Struct(Vec<Value>),
    Null,
}

impl fmt::Display for Value {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Value::I1(b) => write!(f, "{}", b),
            Value::I8(i) => write!(f, "{}", i),
            Value::I16(i) => write!(f, "{}", i),
            Value::I32(i) => write!(f, "{}", i),
            Value::I64(i) => write!(f, "{}", i),
            Value::I128(i) => write!(f, "{}", i),
            Value::F16(bits) => write!(f, "0x{:x}f16", bits),
            Value::F32(bits) => write!(f, "{}f32", f32::from_bits(*bits)),
            Value::F64(bits) => write!(f, "{}f64", f64::from_bits(*bits)),
            Value::F128(bits) => write!(f, "0x{:x}f128", bits),
            Value::Array(vals) => {
                write!(f, "[")?;
                for (i, val) in vals.iter().enumerate() {
                    if i > 0 { write!(f, ", ")?; }
                    write!(f, "{}", val)?;
                }
                write!(f, "]")
            },
            Value::Struct(vals) => {
                write!(f, "{{ ")?;
                for (i, val) in vals.iter().enumerate() {
                    if i > 0 { write!(f, ", ")?; }
                    write!(f, "{}", val)?;
                }
                write!(f, " }}")
            },
            Value::Null => write!(f, "null"),
        }
    }
}

/// Comparison predicates untuk integer
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum IntPredicate {
    EQ,  // Equal
    NE,  // Not equal
    UGT, // Unsigned greater than
    UGE, // Unsigned greater or equal
    ULT, // Unsigned less than
    ULE, // Unsigned less or equal
    SGT, // Signed greater than
    SGE, // Signed greater or equal
    SLT, // Signed less than
    SLE, // Signed less or equal
}

/// Comparison predicates untuk floating point
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum FPPredicate {
    False, // Always false
    OEQ,   // Ordered equal
    OGT,   // Ordered greater than
    OGE,   // Ordered greater or equal
    OLT,   // Ordered less than
    OLE,   // Ordered less or equal
    ONE,   // Ordered not equal
    ORD,   // Ordered (no nans)
    UNO,   // Unordered (either is nan)
    UEQ,   // Unordered equal
    UGT,   // Unordered greater than
    UGE,   // Unordered greater or equal
    ULT,   // Unordered less than
    ULE,   // Unordered less or equal
    UNE,   // Unordered not equal
    TRUE,  // Always true
}

/// Instruction opcode
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum Opcode {
    // Terminator instructions
    Ret,
    Br,
    Switch,
    IndirectBr,
    Invoke,
    Resume,
    Unreachable,

    // Binary operations
    Add,
    FAdd,
    Sub,
    FSub,
    Mul,
    FMul,
    UDiv,
    SDiv,
    FDiv,
    URem,
    SRem,
    FRem,

    // Logical operations
    Shl,
    LShr,
    AShr,
    And,
    Or,
    Xor,

    // Memory operations
    Alloca,
    Load,
    Store,
    GetElementPtr,
    Fence,
    AtomicCmpXchg,
    AtomicRMW,

    // Conversion operations
    Trunc,
    ZExt,
    SExt,
    FPToUI,
    FPToSI,
    UIToFP,
    SIToFP,
    FPTrunc,
    FPExt,
    PtrToInt,
    IntToPtr,
    BitCast,
    AddrSpaceCast,

    // Comparison operations
    ICmp,
    FCmp,

    // Other operations
    PHI,
    Call,
    Select,
    VAArg,
    ExtractElement,
    InsertElement,
    ShuffleVector,
    ExtractValue,
    InsertValue,
    LandingPad,
}

impl fmt::Display for Opcode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Opcode::Ret => write!(f, "ret"),
            Opcode::Br => write!(f, "br"),
            Opcode::Switch => write!(f, "switch"),
            Opcode::IndirectBr => write!(f, "indirectbr"),
            Opcode::Invoke => write!(f, "invoke"),
            Opcode::Resume => write!(f, "resume"),
            Opcode::Unreachable => write!(f, "unreachable"),
            Opcode::Add => write!(f, "add"),
            Opcode::FAdd => write!(f, "fadd"),
            Opcode::Sub => write!(f, "sub"),
            Opcode::FSub => write!(f, "fsub"),
            Opcode::Mul => write!(f, "mul"),
            Opcode::FMul => write!(f, "fmul"),
            Opcode::UDiv => write!(f, "udiv"),
            Opcode::SDiv => write!(f, "sdiv"),
            Opcode::FDiv => write!(f, "fdiv"),
            Opcode::URem => write!(f, "urem"),
            Opcode::SRem => write!(f, "srem"),
            Opcode::FRem => write!(f, "frem"),
            Opcode::Shl => write!(f, "shl"),
            Opcode::LShr => write!(f, "lshr"),
            Opcode::AShr => write!(f, "ashr"),
            Opcode::And => write!(f, "and"),
            Opcode::Or => write!(f, "or"),
            Opcode::Xor => write!(f, "xor"),
            Opcode::Alloca => write!(f, "alloca"),
            Opcode::Load => write!(f, "load"),
            Opcode::Store => write!(f, "store"),
            Opcode::GetElementPtr => write!(f, "getelementptr"),
            Opcode::Fence => write!(f, "fence"),
            Opcode::AtomicCmpXchg => write!(f, "cmpxchg"),
            Opcode::AtomicRMW => write!(f, "atomicrmw"),
            Opcode::Trunc => write!(f, "trunc"),
            Opcode::ZExt => write!(f, "zext"),
            Opcode::SExt => write!(f, "sext"),
            Opcode::FPToUI => write!(f, "fptoui"),
            Opcode::FPToSI => write!(f, "fptosi"),
            Opcode::UIToFP => write!(f, "uitofp"),
            Opcode::SIToFP => write!(f, "sitofp"),
            Opcode::FPTrunc => write!(f, "fptrunc"),
            Opcode::FPExt => write!(f, "fpext"),
            Opcode::PtrToInt => write!(f, "ptrtoint"),
            Opcode::IntToPtr => write!(f, "inttoptr"),
            Opcode::BitCast => write!(f, "bitcast"),
            Opcode::AddrSpaceCast => write!(f, "addrspacecast"),
            Opcode::ICmp => write!(f, "icmp"),
            Opcode::FCmp => write!(f, "fcmp"),
            Opcode::PHI => write!(f, "phi"),
            Opcode::Call => write!(f, "call"),
            Opcode::Select => write!(f, "select"),
            Opcode::VAArg => write!(f, "va_arg"),
            Opcode::ExtractElement => write!(f, "extractelement"),
            Opcode::InsertElement => write!(f, "insertelement"),
            Opcode::ShuffleVector => write!(f, "shufflevector"),
            Opcode::ExtractValue => write!(f, "extractvalue"),
            Opcode::InsertValue => write!(f, "insertvalue"),
            Opcode::LandingPad => write!(f, "landingpad"),
        }
    }
}

/// Instruction dengan operand dan tipe
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct Instruction {
    pub opcode: Opcode,
    pub result_name: Option<String>,
    pub result_type: Option<Type>,
    pub operands: Vec<Operand>,
    pub metadata: Option<Metadata>,
}

/// Metadata untuk instruction
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct Metadata {
    pub debug_loc: Option<DebugLocation>,
    pub tbaa: Option<TBAAMetadata>,
    pub alias_scope: Option<String>,
    pub noalias: Option<String>,
}

/// Debug location information
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct DebugLocation {
    pub line: u32,
    pub column: u32,
    pub scope: String,
    pub inlined_at: Option<String>,
}

/// Type Based Alias Analysis metadata
#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct TBAAMetadata {
    pub type_id: String,
    pub parent: Option<String>,
}

impl Instruction {
    /// Buat instruction baru
    pub fn new(opcode: Opcode, result_name: Option<String>, result_type: Option<Type>, operands: Vec<Operand>) -> Self {
        Instruction {
            opcode,
            result_name,
            result_type,
            operands,
            metadata: None,
        }
    }

    /// Tambahkan metadata
    pub fn with_metadata(mut self, metadata: Metadata) -> Self {
        self.metadata = Some(metadata);
        self
    }

    /// Validasi instruction
    pub fn validate(&self) -> Result<()> {
        // Validasi operand count untuk setiap opcode
        let expected_operands = match self.opcode {
            Opcode::Ret => 0..=1,
            Opcode::Br => 1..=3,
            Opcode::Add | Opcode::FAdd | Opcode::Sub | Opcode::FSub |
            Opcode::Mul | Opcode::FMul | Opcode::UDiv | Opcode::SDiv |
            Opcode::FDiv | Opcode::URem | Opcode::SRem | Opcode::FRem => 2..=2,
            Opcode::Alloca => 0..=2,
            Opcode::Load => 1..=2,
            Opcode::Store => 2..=3,
            Opcode::ICmp | Opcode::FCmp => 3..=3,
            Opcode::Call => 1..=usize::MAX,
            _ => 0..=usize::MAX,
        };

        if !expected_operands.contains(&self.operands.len()) {
            return Err(CoreError::InvalidInstruction(format!(
                "Opcode {} expects {} operands, got {}",
                self.opcode,
                expected_operands.start(),
                self.operands.len()
            )));
        }

        // Validasi tipe hasil
        if let Some(ref ty) = self.result_type {
            ty.validate()?;
        }

        Ok(())
    }

    /// Ambil opcode
    pub fn opcode(&self) -> &Opcode {
        &self.opcode
    }

    /// Ambil operand
    pub fn operands(&self) -> &[Operand] {
        &self.operands
    }

    /// Apakah instruction ini terminator?
    pub fn is_terminator(&self) -> bool {
        matches!(self.opcode, 
            Opcode::Ret | Opcode::Br | Opcode::Switch | 
            Opcode::IndirectBr | Opcode::Invoke | Opcode::Resume | Opcode::Unreachable)
    }

    /// Apakah instruction ini binary operation?
    pub fn is_binary_op(&self) -> bool {
        matches!(self.opcode,
            Opcode::Add | Opcode::FAdd | Opcode::Sub | Opcode::FSub |
            Opcode::Mul | Opcode::FMul | Opcode::UDiv | Opcode::SDiv |
            Opcode::FDiv | Opcode::URem | Opcode::SRem | Opcode::FRem |
            Opcode::Shl | Opcode::LShr | Opcode::AShr | Opcode::And |
            Opcode::Or | Opcode::Xor)
    }

    /// Apakah instruction ini memory operation?
    pub fn is_memory_op(&self) -> bool {
        matches!(self.opcode,
            Opcode::Alloca | Opcode::Load | Opcode::Store |
            Opcode::GetElementPtr | Opcode::Fence |
            Opcode::AtomicCmpXchg | Opcode::AtomicRMW)
    }
}

impl fmt::Display for Instruction {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if let Some(ref name) = self.result_name {
            write!(f, "%{} = ", name)?;
        }
        
        write!(f, "{}", self.opcode)?;
        
        if let Some(ref ty) = self.result_type {
            write!(f, " {}", ty)?;
        }
        
        if !self.operands.is_empty() {
            write!(f, " ")?;
            for (i, operand) in self.operands.iter().enumerate() {
                if i > 0 { write!(f, ", ")?; }
                write!(f, "{}", operand)?;
            }
        }
        
        Ok(())
    }
}

/// Builder untuk instruction
pub struct InstructionBuilder {
    opcode: Opcode,
    result_name: Option<String>,
    result_type: Option<Type>,
    operands: Vec<Operand>,
    metadata: Option<Metadata>,
}

impl InstructionBuilder {
    pub fn new(opcode: Opcode) -> Self {
        InstructionBuilder {
            opcode,
            result_name: None,
            result_type: None,
            operands: Vec::new(),
            metadata: None,
        }
    }

    pub fn result(mut self, name: String, ty: Type) -> Self {
        self.result_name = Some(name);
        self.result_type = Some(ty);
        self
    }

    pub fn result_type(mut self, ty: Type) -> Self {
        self.result_type = Some(ty);
        self
    }

    pub fn result_name(mut self, name: String) -> Self {
        self.result_name = Some(name);
        self
    }

    pub fn operand(mut self, operand: Operand) -> Self {
        self.operands.push(operand);
        self
    }

    pub fn operands(mut self, operands: Vec<Operand>) -> Self {
        self.operands = operands;
        self
    }

    pub fn metadata(mut self, metadata: Metadata) -> Self {
        self.metadata = Some(metadata);
        self
    }

    pub fn build(self) -> Instruction {
        Instruction {
            opcode: self.opcode,
            result_name: self.result_name,
            result_type: self.result_type,
            operands: self.operands,
            metadata: self.metadata,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_instruction_creation() {
        let inst = InstructionBuilder::new(Opcode::Add)
            .result_type(Type::Primitive(PrimitiveType::I32))
            .operand(Operand::Register("a".to_string()))
            .operand(Operand::Register("b".to_string()))
            .build();

        assert_eq!(inst.opcode, Opcode::Add);
        assert!(inst.result_type.is_some());
        assert_eq!(inst.operands.len(), 2);
        assert!(inst.is_binary_op());
    }

    #[test]
    fn test_instruction_validation() {
        let valid_inst = InstructionBuilder::new(Opcode::Add)
            .result_type(Type::Primitive(PrimitiveType::I32))
            .operand(Operand::Register("a".to_string()))
            .operand(Operand::Register("b".to_string()))
            .build();

        assert!(valid_inst.validate().is_ok());

        let invalid_inst = InstructionBuilder::new(Opcode::Add)
            .result_type(Type::Primitive(PrimitiveType::I32))
            .operand(Operand::Register("a".to_string()))
            .build();

        assert!(invalid_inst.validate().is_err());
    }
}