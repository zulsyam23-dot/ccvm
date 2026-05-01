//! Module system untuk CCVM IR

use serde::{Serialize, Deserialize};
use std::collections::HashMap;
use crate::{types::*, instruction::*, CoreError, Result};

/// Sebuah module CCVM yang berisi functions, globals, dan metadata
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Module {
    /// Nama module
    pub name: String,
    
    /// Target triple (e.g., "x86_64-pc-linux-gnu")
    pub target_triple: Option<String>,
    
    /// Data layout string
    pub data_layout: Option<String>,
    
    /// Functions dalam module
    pub functions: HashMap<String, Function>,
    
    /// Global variables
    pub globals: HashMap<String, GlobalVariable>,
    
    /// Struct type definitions
    pub struct_types: HashMap<String, StructType>,
    
    /// Metadata
    pub metadata: HashMap<String, MetadataNode>,
}

/// Function dalam module
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Function {
    /// Nama function
    pub name: String,
    
    /// Tipe function
    pub function_type: FunctionType,
    
    /// Basic blocks dalam function
    pub basic_blocks: Vec<BasicBlock>,
    
    /// Linkage type
    pub linkage: LinkageType,
    
    /// Calling convention
    pub calling_convention: CallingConvention,
    
    /// Attributes
    pub attributes: Vec<FunctionAttribute>,
    
    /// Visibility
    pub visibility: Visibility,
}

/// Basic block dalam function
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct BasicBlock {
    /// Label basic block
    pub label: String,
    
    /// Instructions dalam basic block
    pub instructions: Vec<Instruction>,
    
    /// Predecessors (untuk analisis)
    pub predecessors: Vec<String>,
    
    /// Successors (untuk analisis)
    pub successors: Vec<String>,
}

/// Global variable
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct GlobalVariable {
    /// Nama variable
    pub name: String,
    
    /// Tipe variable
    pub variable_type: Type,
    
    /// Initializer
    pub initializer: Option<Constant>,
    
    /// Linkage type
    pub linkage: LinkageType,
    
    /// Alignment
    pub alignment: u32,
    
    /// Section
    pub section: Option<String>,
    
    /// Visibility
    pub visibility: Visibility,
}

/// Constant value
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub enum Constant {
    Scalar(Value),
    Array(Vec<Constant>),
    Struct(Vec<Constant>),
    GlobalRef(String),
    ZeroInitializer,
}

/// Linkage types
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum LinkageType {
    Private,
    Internal,
    AvailableExternally,
    LinkOnce,
    Weak,
    Common,
    Appending,
    ExternWeak,
    External,
}

/// Calling conventions
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum CallingConvention {
    C,              // C calling convention
    Fast,           // Fast calling convention
    Cold,           // Cold calling convention
    GHC,            // Glasgow Haskell Compiler
    HiPE,           // High Performance Erlang
    WebKitJS,       // WebKit JavaScript
    AnyReg,         // Any register calling convention
    PreserveMost,   // Preserve most registers
    PreserveAll,    // Preserve all registers
    Swift,          // Swift calling convention
    CXXFastTLS,     // C++ Fast TLS
    FirstTargetCC,  // First target-specific calling convention
}

/// Function attributes
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum FunctionAttribute {
    AlwaysInline,
    Builtin,
    Cold,
    Convergent,
    InlineHint,
    JumpTable,
    MinSize,
    Naked,
    NoBuiltin,
    NoDuplicate,
    NoImplicitFloat,
    NoInline,
    NoRecurse,
    NoRedZone,
    NoReturn,
    NoUnwind,
    NonLazyBind,
    NonNull,
    OptForFuzzing,
    OptNone,
    OptSize,
    ReadNone,
    ReadOnly,
    ReturnsTwice,
    SafeStack,
    SanitizeAddress,
    SanitizeMemory,
    SanitizeThread,
    Speculatable,
    Ssp,
    SspReq,
    SspStrong,
    StrictFP,
    UWTable,
    WriteOnly,
}

/// Visibility
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub enum Visibility {
    Default,
    Hidden,
    Protected,
}

/// Metadata node
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct MetadataNode {
    pub kind: String,
    pub operands: Vec<MetadataOperand>,
}

/// Metadata operand
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub enum MetadataOperand {
    String(String),
    Integer(i64),
    Node(String),
}

impl Module {
    /// Buat module baru
    pub fn new(name: String) -> Self {
        Module {
            name,
            target_triple: None,
            data_layout: None,
            functions: HashMap::new(),
            globals: HashMap::new(),
            struct_types: HashMap::new(),
            metadata: HashMap::new(),
        }
    }

    /// Tambahkan function ke module
    pub fn add_function(&mut self, function: Function) -> Result<()> {
        if self.functions.contains_key(&function.name) {
            return Err(CoreError::ValidationError(format!(
                "Function '{}' already exists in module",
                function.name
            )));
        }
        self.functions.insert(function.name.clone(), function);
        Ok(())
    }

    /// Ambil function dari module
    pub fn get_function(&self, name: &str) -> Option<&Function> {
        self.functions.get(name)
    }

    /// Tambahkan global variable
    pub fn add_global(&mut self, global: GlobalVariable) -> Result<()> {
        if self.globals.contains_key(&global.name) {
            return Err(CoreError::ValidationError(format!(
                "Global variable '{}' already exists in module",
                global.name
            )));
        }
        self.globals.insert(global.name.clone(), global);
        Ok(())
    }

    /// Ambil global variable
    pub fn get_global(&self, name: &str) -> Option<&GlobalVariable> {
        self.globals.get(name)
    }

    /// Tambahkan struct type
    pub fn add_struct_type(&mut self, name: String, struct_type: StructType) -> Result<()> {
        if self.struct_types.contains_key(&name) {
            return Err(CoreError::ValidationError(format!(
                "Struct type '{}' already exists in module",
                name
            )));
        }
        self.struct_types.insert(name, struct_type);
        Ok(())
    }

    /// Ambil struct type
    pub fn get_struct_type(&self, name: &str) -> Option<&StructType> {
        self.struct_types.get(name)
    }

    /// Validasi module
    pub fn validate(&self) -> Result<()> {
        // Validasi functions
        for function in self.functions.values() {
            function.validate()?;
        }

        // Validasi globals
        for global in self.globals.values() {
            global.validate()?;
        }

        // Validasi struct types
        for struct_type in self.struct_types.values() {
            struct_type.validate()?;
        }

        Ok(())
    }

    /// Ambil semua function names
    pub fn function_names(&self) -> Vec<&String> {
        self.functions.keys().collect()
    }

    /// Ambil semua global names
    pub fn global_names(&self) -> Vec<&String> {
        self.globals.keys().collect()
    }

    /// Ambil semua struct type names
    pub fn struct_type_names(&self) -> Vec<&String> {
        self.struct_types.keys().collect()
    }
}

impl Function {
    /// Buat function baru
    pub fn new(name: String, function_type: FunctionType) -> Self {
        Function {
            name,
            function_type,
            basic_blocks: Vec::new(),
            linkage: LinkageType::External,
            calling_convention: CallingConvention::C,
            attributes: Vec::new(),
            visibility: Visibility::Default,
        }
    }

    /// Tambahkan basic block
    pub fn add_basic_block(&mut self, block: BasicBlock) {
        self.basic_blocks.push(block);
    }

    /// Ambil basic block berdasarkan label
    pub fn get_basic_block(&self, label: &str) -> Option<&BasicBlock> {
        self.basic_blocks.iter().find(|bb| bb.label == label)
    }

    /// Ambil basic block berdasarkan index
    pub fn get_basic_block_mut(&mut self, index: usize) -> Option<&mut BasicBlock> {
        self.basic_blocks.get_mut(index)
    }

    /// Validasi function
    pub fn validate(&self) -> Result<()> {
        if self.basic_blocks.is_empty() {
            return Err(CoreError::ValidationError(format!(
                "Function '{}' has no basic blocks",
                self.name
            )));
        }

        // Validasi setiap basic block
        for block in &self.basic_blocks {
            block.validate()?;
        }

        // Validasi CFG
        self.validate_cfg()?;

        Ok(())
    }

    /// Validasi Control Flow Graph
    fn validate_cfg(&self) -> Result<()> {
        let block_labels: std::collections::HashSet<&str> = 
            self.basic_blocks.iter().map(|bb| bb.label.as_str()).collect();

        for block in &self.basic_blocks {
            for successor in &block.successors {
                if !block_labels.contains(successor.as_str()) {
                    return Err(CoreError::ValidationError(format!(
                        "Basic block '{}' has invalid successor '{}'",
                        block.label, successor
                    )));
                }
            }
        }

        Ok(())
    }

    /// Apakah function ini declaration only?
    pub fn is_declaration(&self) -> bool {
        self.basic_blocks.is_empty()
    }

    /// Apakah function ini definition?
    pub fn is_definition(&self) -> bool {
        !self.basic_blocks.is_empty()
    }
}

impl BasicBlock {
    /// Buat basic block baru
    pub fn new(label: String) -> Self {
        BasicBlock {
            label,
            instructions: Vec::new(),
            predecessors: Vec::new(),
            successors: Vec::new(),
        }
    }

    /// Tambahkan instruction
    pub fn add_instruction(&mut self, instruction: Instruction) {
        self.instructions.push(instruction);
    }

    /// Ambil terminator instruction (last instruction)
    pub fn get_terminator(&self) -> Option<&Instruction> {
        self.instructions.last().filter(|inst| inst.is_terminator())
    }

    /// Validasi basic block
    pub fn validate(&self) -> Result<()> {
        if self.instructions.is_empty() {
            return Err(CoreError::ValidationError(format!(
                "Basic block '{}' has no instructions",
                self.label
            )));
        }

        // Basic block harus diakhiri dengan terminator
        if !self.instructions.last().unwrap().is_terminator() {
            return Err(CoreError::ValidationError(format!(
                "Basic block '{}' does not end with terminator",
                self.label
            )));
        }

        // Validasi setiap instruction
        for instruction in &self.instructions {
            instruction.validate()?;
        }

        Ok(())
    }

    /// Ambil jumlah instructions
    pub fn instruction_count(&self) -> usize {
        self.instructions.len()
    }
}

impl GlobalVariable {
    /// Buat global variable baru
    pub fn new(name: String, variable_type: Type) -> Self {
        GlobalVariable {
            name,
            variable_type,
            initializer: None,
            linkage: LinkageType::External,
            alignment: 0,
            section: None,
            visibility: Visibility::Default,
        }
    }

    /// Set initializer
    pub fn with_initializer(mut self, initializer: Constant) -> Self {
        self.initializer = Some(initializer);
        self
    }

    /// Set alignment
    pub fn with_alignment(mut self, alignment: u32) -> Self {
        self.alignment = alignment;
        self
    }

    /// Validasi global variable
    pub fn validate(&self) -> Result<()> {
        self.variable_type.validate()?;

        if let Some(ref init) = self.initializer {
            self.validate_initializer(init)?;
        }

        Ok(())
    }

    /// Validasi initializer
    fn validate_initializer(&self, _initializer: &Constant) -> Result<()> {
        // TODO: Implementasi validasi tipe initializer
        Ok(())
    }
}

impl StructType {
    /// Validasi struct type
    pub fn validate(&self) -> Result<()> {
        if self.fields.is_empty() {
            return Err(CoreError::ValidationError("Struct must have at least one field".to_string()));
        }

        for (_, field_ty) in &self.fields {
            field_ty.validate()?;
        }

        Ok(())
    }

    /// Hitung ukuran struct dalam bytes
    pub fn size(&self) -> u32 {
        self.fields.iter()
            .map(|(_, ty)| ty.byte_size())
            .sum()
    }

    /// Hitung alignment struct
    pub fn alignment(&self) -> u32 {
        self.fields.iter()
            .map(|(_, ty)| ty.alignment())
            .max()
            .unwrap_or(1)
    }

    /// Ambil field berdasarkan index
    pub fn get_field(&self, index: usize) -> Option<&(String, Type)> {
        self.fields.get(index)
    }

    /// Ambil field berdasarkan nama
    pub fn get_field_by_name(&self, name: &str) -> Option<&(String, Type)> {
        self.fields.iter().find(|(field_name, _)| field_name == name)
    }
}

/// Builder untuk module
pub struct ModuleBuilder {
    module: Module,
}

impl ModuleBuilder {
    pub fn new(name: String) -> Self {
        ModuleBuilder {
            module: Module::new(name),
        }
    }

    pub fn target_triple(mut self, triple: String) -> Self {
        self.module.target_triple = Some(triple);
        self
    }

    pub fn data_layout(mut self, layout: String) -> Self {
        self.module.data_layout = Some(layout);
        self
    }

    pub fn function(mut self, function: Function) -> Self {
        self.module.add_function(function).unwrap();
        self
    }

    pub fn global(mut self, global: GlobalVariable) -> Self {
        self.module.add_global(global).unwrap();
        self
    }

    pub fn struct_type(mut self, name: String, struct_type: StructType) -> Self {
        self.module.add_struct_type(name, struct_type).unwrap();
        self
    }

    pub fn build(self) -> Module {
        self.module
    }
}

/// Builder untuk function
pub struct FunctionBuilder {
    function: Function,
}

impl FunctionBuilder {
    pub fn new(name: String, function_type: FunctionType) -> Self {
        FunctionBuilder {
            function: Function::new(name, function_type),
        }
    }

    pub fn linkage(mut self, linkage: LinkageType) -> Self {
        self.function.linkage = linkage;
        self
    }

    pub fn calling_convention(mut self, cc: CallingConvention) -> Self {
        self.function.calling_convention = cc;
        self
    }

    pub fn visibility(mut self, visibility: Visibility) -> Self {
        self.function.visibility = visibility;
        self
    }

    pub fn attribute(mut self, attr: FunctionAttribute) -> Self {
        self.function.attributes.push(attr);
        self
    }

    pub fn basic_block(mut self, block: BasicBlock) -> Self {
        self.function.add_basic_block(block);
        self
    }

    pub fn build(self) -> Function {
        self.function
    }
}

/// Builder untuk basic block
pub struct BasicBlockBuilder {
    block: BasicBlock,
}

impl BasicBlockBuilder {
    pub fn new(label: String) -> Self {
        BasicBlockBuilder {
            block: BasicBlock::new(label),
        }
    }

    pub fn instruction(mut self, instruction: Instruction) -> Self {
        self.block.add_instruction(instruction);
        self
    }

    pub fn instructions(mut self, instructions: Vec<Instruction>) -> Self {
        for inst in instructions {
            self.block.add_instruction(inst);
        }
        self
    }

    pub fn predecessor(mut self, pred: String) -> Self {
        self.block.predecessors.push(pred);
        self
    }

    pub fn successor(mut self, succ: String) -> Self {
        self.block.successors.push(succ);
        self
    }

    pub fn build(self) -> BasicBlock {
        self.block
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_module_creation() {
        let mut module = Module::new("test".to_string());
        
        let func_type = FunctionType {
            return_type: Box::new(Type::Primitive(PrimitiveType::I32)),
            parameter_types: vec![],
            is_vararg: false,
        };
        
        let function = Function::new("main".to_string(), func_type);
        module.add_function(function).unwrap();
        
        assert_eq!(module.function_names().len(), 1);
        assert!(module.get_function("main").is_some());
    }

    #[test]
    fn test_function_validation() {
        let func_type = FunctionType {
            return_type: Box::new(Type::Primitive(PrimitiveType::I32)),
            parameter_types: vec![],
            is_vararg: false,
        };
        
        let mut function = Function::new("test".to_string(), func_type);
        
        // Function tanpa basic blocks harusnya invalid
        assert!(function.validate().is_err());
        
        // Tambahkan basic block dengan terminator
        let mut block = BasicBlock::new("entry".to_string());
        let ret_inst = InstructionBuilder::new(crate::instruction::Opcode::Ret)
            .result_type(Type::Primitive(PrimitiveType::I32))
            .operand(Operand::Immediate(Value::I32(0)))
            .build();
        block.add_instruction(ret_inst);
        function.add_basic_block(block);
        
        // Sekarang harusnya valid
        assert!(function.validate().is_ok());
    }

    #[test]
    fn test_basic_block_validation() {
        let mut block = BasicBlock::new("entry".to_string());
        
        // Basic block tanpa instruction harusnya invalid
        assert!(block.validate().is_err());
        
        // Tambahkan terminator instruction
        let ret_inst = InstructionBuilder::new(crate::instruction::Opcode::Ret)
            .result_type(Type::Primitive(PrimitiveType::I32))
            .operand(Operand::Immediate(Value::I32(0)))
            .build();
        block.add_instruction(ret_inst);
        
        // Sekarang harusnya valid
        assert!(block.validate().is_ok());
    }
}