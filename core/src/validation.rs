//! Validation framework untuk CCVM IR

use crate::{types::*, instruction::*, module::*, CoreError, Result};
use std::collections::{HashMap, HashSet};

/// Validator untuk module
pub struct ModuleValidator {
    module: Module,
    errors: Vec<ValidationError>,
}

#[derive(Debug, Clone)]
pub struct ValidationError {
    pub kind: ValidationErrorKind,
    pub message: String,
    pub location: Option<String>,
}

#[derive(Debug, Clone)]
pub enum ValidationErrorKind {
    TypeError,
    UndefinedSymbol,
    InvalidInstruction,
    InvalidControlFlow,
    InvalidMemoryAccess,
    SSAViolation,
}

impl ModuleValidator {
    /// Buat validator baru
    pub fn new(module: Module) -> Self {
        ModuleValidator {
            module,
            errors: Vec::new(),
        }
    }

    /// Jalankan validasi lengkap
    pub fn validate(mut self) -> Result<Module> {
        self.validate_module_structure();
        self.validate_types();
        self.validate_functions();
        self.validate_globals();
        self.validate_control_flow();
        self.validate_ssa_form();
        self.validate_memory_accesses();

        if !self.errors.is_empty() {
            let error_messages: Vec<String> = self.errors.iter()
                .map(|e| format!("{:?}: {}", e.kind, e.message))
                .collect();
            return Err(CoreError::ValidationError(error_messages.join("\n")));
        }

        Ok(self.module)
    }

    /// Validasi struktur module
    fn validate_module_structure(&mut self) {
        // Validasi nama module
        if self.module.name.is_empty() {
            self.add_error(ValidationErrorKind::InvalidInstruction, 
                "Module name cannot be empty".to_string(), None);
        }

        // Validasi target triple format
        if let Some(ref triple) = self.module.target_triple {
            if !self.is_valid_target_triple(triple) {
                self.add_error(ValidationErrorKind::TypeError,
                    format!("Invalid target triple format: {}", triple),
                    None);
            }
        }
    }

    /// Validasi semua tipe
    fn validate_types(&mut self) {
        let struct_types: Vec<_> = self.module.struct_types.values().cloned().collect();
        for struct_type in struct_types {
            if let Err(e) = struct_type.validate() {
                self.add_error(ValidationErrorKind::TypeError,
                    format!("Invalid struct type: {}", e),
                    None);
            }
        }
    }

    /// Validasi functions
    fn validate_functions(&mut self) {
        let function_names: Vec<String> = self.module.functions.keys().cloned().collect();
        for name in function_names {
            let function = self.module.functions.get(&name).unwrap().clone();
            self.validate_function(&function);
        }
    }

    /// Validasi single function
    fn validate_function(&mut self, function: &Function) {
        let func_location = format!("function '{}'", function.name);

        // Validasi function type
        if let Err(e) = function.function_type.return_type.validate() {
            self.add_error(ValidationErrorKind::TypeError,
                format!("Invalid return type: {}", e),
                Some(func_location.clone()));
        }

        for (i, param_ty) in function.function_type.parameter_types.iter().enumerate() {
            if let Err(e) = param_ty.validate() {
                self.add_error(ValidationErrorKind::TypeError,
                    format!("Invalid parameter type at index {}: {}", i, e),
                    Some(func_location.clone()));
            }
        }

        // Validasi basic blocks
        if function.basic_blocks.is_empty() {
            self.add_error(ValidationErrorKind::InvalidControlFlow,
                "Function definition has no basic blocks".to_string(),
                Some(func_location));
        }

        // We can't pass function as ref and self as mut at the same time if we access self.module inside validate_basic_block
        // But validate_basic_block only uses function and itself.
        // Let's copy blocks if needed or just be careful.
        let blocks = function.basic_blocks.clone();
        for block in &blocks {
            self.validate_basic_block(function, block);
        }
    }

    /// Validasi basic block
    fn validate_basic_block(&mut self, function: &Function, block: &BasicBlock) {
        let block_location = format!("function '{}' block '{}'", function.name, block.label);

        if block.instructions.is_empty() {
            self.add_error(ValidationErrorKind::InvalidInstruction,
                "Basic block has no instructions".to_string(),
                Some(block_location.clone()));
            return;
        }

        // Validasi terminator
        if let Some(last_inst) = block.instructions.last() {
            if !last_inst.is_terminator() {
                self.add_error(ValidationErrorKind::InvalidControlFlow,
                    "Basic block does not end with terminator instruction".to_string(),
                    Some(block_location.clone()));
            }
        }

        // Validasi setiap instruction
        for (i, instruction) in block.instructions.iter().enumerate() {
            let inst_location = format!("{} instruction {}", block_location, i);
            self.validate_instruction(function, instruction, &inst_location);
        }
    }

    /// Validasi instruction
    fn validate_instruction(&mut self, function: &Function, instruction: &Instruction, location: &str) {
        // Validasi instruction structure
        if let Err(e) = instruction.validate() {
            self.add_error(ValidationErrorKind::InvalidInstruction,
                format!("Invalid instruction: {}", e),
                Some(location.to_string()));
        }

        // Validasi operand types
        self.validate_instruction_operands(function, instruction, location);

        // Validasi memory operations
        if instruction.is_memory_op() {
            self.validate_memory_instruction(instruction, location);
        }
    }

    /// Validasi operand instruction
    fn validate_instruction_operands(&mut self, function: &Function, instruction: &Instruction, location: &str) {
        for operand in &instruction.operands {
            match operand {
                crate::instruction::Operand::Register(_reg) => {
                    // Validasi register usage akan dilakukan di SSA validation
                },
                crate::instruction::Operand::Global(name) => {
                    if !self.module.globals.contains_key(name) && 
                       !self.module.functions.contains_key(name) {
                        self.add_error(ValidationErrorKind::UndefinedSymbol,
                            format!("Undefined global symbol: {}", name),
                            Some(location.to_string()));
                    }
                },
                crate::instruction::Operand::Label(label) => {
                    if !function.basic_blocks.iter().any(|bb| &bb.label == label) {
                        self.add_error(ValidationErrorKind::UndefinedSymbol,
                            format!("Undefined basic block label: {}", label),
                            Some(location.to_string()));
                    }
                },
                _ => {}
            }
        }
    }

    /// Validasi memory instruction
    fn validate_memory_instruction(&mut self, instruction: &Instruction, location: &str) {
        match instruction.opcode {
            crate::instruction::Opcode::Load => {
                // Validasi load instruction
                if instruction.operands.is_empty() {
                    self.add_error(ValidationErrorKind::InvalidMemoryAccess,
                        "Load instruction requires pointer operand".to_string(),
                        Some(location.to_string()));
                }
            },
            crate::instruction::Opcode::Store => {
                // Validasi store instruction
                if instruction.operands.len() < 2 {
                    self.add_error(ValidationErrorKind::InvalidMemoryAccess,
                        "Store instruction requires value and pointer operands".to_string(),
                        Some(location.to_string()));
                }
            },
            _ => {}
        }
    }

    /// Validasi globals
    fn validate_globals(&mut self) {
        let globals: Vec<_> = self.module.globals.values().cloned().collect();
        for global in globals {
            if let Err(e) = global.validate() {
                self.add_error(ValidationErrorKind::TypeError,
                    format!("Invalid global variable '{}': {}", global.name, e),
                    None);
            }
        }
    }

    /// Validasi control flow graph
    fn validate_control_flow(&mut self) {
        let function_names: Vec<String> = self.module.functions.keys().cloned().collect();
        for name in function_names {
            let function = self.module.functions.get(&name).unwrap().clone();
            self.validate_function_cfg(&function);
        }
    }

    /// Validasi function CFG
    fn validate_function_cfg(&mut self, function: &Function) {
        let mut block_map = HashMap::new();
        let mut predecessors: HashMap<String, HashSet<String>> = HashMap::new();

        // Build block map dan predecessors
        for block in &function.basic_blocks {
            block_map.insert(block.label.clone(), block);
            
            for succ in &block.successors {
                predecessors.entry(succ.clone())
                    .or_insert_with(HashSet::new)
                    .insert(block.label.clone());
            }
        }

        // Validasi predecessors consistency
        for block in &function.basic_blocks {
            let expected_preds = predecessors.get(&block.label)
                .map(|s| s.len()).unwrap_or(0);
            let actual_preds = block.predecessors.len();

            if expected_preds != actual_preds {
                self.add_error(ValidationErrorKind::InvalidControlFlow,
                    format!("Basic block '{}' has {} predecessors but expected {}",
                        block.label, actual_preds, expected_preds),
                    Some(format!("function '{}'", function.name)));
            }
        }

        // Validasi reachable blocks (semua blocks harus reachable dari entry)
        let entry_block = function.basic_blocks.first();
        if let Some(entry) = entry_block {
            let reachable = self.compute_reachable_blocks(function, &entry.label);
            
            for block in &function.basic_blocks {
                if !reachable.contains(&block.label) {
                    self.add_error(ValidationErrorKind::InvalidControlFlow,
                        format!("Basic block '{}' is not reachable from entry", block.label),
                        Some(format!("function '{}'", function.name)));
                }
            }
        }
    }

    /// Compute reachable blocks dari entry
    fn compute_reachable_blocks(&self, function: &Function, entry: &str) -> HashSet<String> {
        let mut reachable = HashSet::new();
        let mut worklist = vec![entry.to_string()];

        while let Some(current) = worklist.pop() {
            if reachable.contains(&current) {
                continue;
            }

            reachable.insert(current.clone());

            if let Some(block) = function.basic_blocks.iter().find(|bb| bb.label == current) {
                for succ in &block.successors {
                    worklist.push(succ.clone());
                }
            }
        }

        reachable
    }

    /// Validasi SSA form
    fn validate_ssa_form(&mut self) {
        let function_names: Vec<String> = self.module.functions.keys().cloned().collect();
        for name in function_names {
            let function = self.module.functions.get(&name).unwrap().clone();
            self.validate_function_ssa(&function);
        }
    }

    /// Validasi SSA form untuk function
    fn validate_function_ssa(&mut self, function: &Function) {
        let mut defined_registers = HashSet::new();
        let mut used_registers = HashSet::new();

        for block in &function.basic_blocks {
            for instruction in &block.instructions {
                // Collect defined registers (result register)
                if let Some(result_reg) = self.get_result_register(instruction) {
                    if defined_registers.contains(&result_reg) {
                        self.add_error(ValidationErrorKind::SSAViolation,
                            format!("Register '{}' is defined multiple times", result_reg),
                            Some(format!("function '{}' block '{}'", function.name, block.label)));
                    }
                    defined_registers.insert(result_reg);
                }

                // Collect used registers
                for operand in &instruction.operands {
                    if let crate::instruction::Operand::Register(reg) = operand {
                        used_registers.insert(reg.clone());
                    }
                }
            }
        }

        // Validasi semua used registers didefinisikan
        for used_reg in &used_registers {
            if !defined_registers.contains(used_reg) {
                self.add_error(ValidationErrorKind::SSAViolation,
                    format!("Register '{}' is used but not defined", used_reg),
                    Some(format!("function '{}'", function.name)));
            }
        }
    }

    /// Ambil result register dari instruction
    fn get_result_register(&self, instruction: &Instruction) -> Option<String> {
        instruction.result_name.clone()
    }

    /// Validasi memory accesses
    fn validate_memory_accesses(&mut self) {
        let function_names: Vec<String> = self.module.functions.keys().cloned().collect();
        for name in function_names {
            let function = self.module.functions.get(&name).unwrap().clone();
            self.validate_function_memory_accesses(&function);
        }
    }

    /// Validasi memory accesses dalam function
    fn validate_function_memory_accesses(&mut self, function: &Function) {
        for block in &function.basic_blocks {
            for instruction in &block.instructions {
                self.validate_instruction_memory_access(function, instruction, &block.label);
            }
        }
    }

    /// Validasi memory access untuk single instruction
    fn validate_instruction_memory_access(&mut self, _function: &Function, 
                                        instruction: &Instruction, block_label: &str) {
        match instruction.opcode {
            crate::instruction::Opcode::Load => {
                // Validasi load alignment
                self.validate_memory_alignment(instruction, "load", block_label);
            },
            crate::instruction::Opcode::Store => {
                // Validasi store alignment
                self.validate_memory_alignment(instruction, "store", block_label);
            },
            crate::instruction::Opcode::Alloca => {
                // Validasi alloca size
                if instruction.operands.len() > 1 {
                    // TODO: Validasi size operand
                }
            },
            _ => {}
        }
    }

    /// Validasi memory alignment
    fn validate_memory_alignment(&mut self, instruction: &Instruction, 
                               operation: &str, block_label: &str) {
        // Basic alignment check: size must be power of 2 for primitive types
        if let Some(ref ty) = instruction.result_type {
            let size = match ty {
                Type::Primitive(p) => match p {
                    PrimitiveType::I1 => 1,
                    PrimitiveType::I8 => 1,
                    PrimitiveType::I16 => 2,
                    PrimitiveType::I32 => 4,
                    PrimitiveType::I64 => 8,
                    _ => 0,
                },
                _ => 0,
            };
            
            if size > 0 && (size & (size - 1)) != 0 {
                self.add_error(ValidationErrorKind::InvalidMemoryAccess,
                    format!("Invalid alignment for {} operation in block {}", operation, block_label),
                    None);
            }
        }
    }

    /// Validasi target triple format
    fn is_valid_target_triple(&self, triple: &str) -> bool {
        // Format: arch-vendor-os-env
        let parts: Vec<&str> = triple.split('-').collect();
        parts.len() >= 3 && !parts[0].is_empty() && !parts[1].is_empty() && !parts[2].is_empty()
    }

    /// Tambahkan error ke list
    fn add_error(&mut self, kind: ValidationErrorKind, message: String, location: Option<String>) {
        self.errors.push(ValidationError {
            kind,
            message,
            location,
        });
    }
}

/// Utility functions untuk validasi
pub fn validate_module(module: Module) -> Result<Module> {
    let validator = ModuleValidator::new(module);
    validator.validate()
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::module::*;
    use crate::instruction::*;
    use crate::types::*;

    #[test]
    fn test_valid_module() {
        let mut module = Module::new("test".to_string());
        
        // Buat function type
        let func_type = FunctionType {
            return_type: Box::new(Type::Primitive(PrimitiveType::I32)),
            parameter_types: vec![],
            is_vararg: false,
        };
        
        // Buat function
        let mut function = Function::new("main".to_string(), func_type);
        
        // Buat basic block dengan return instruction
        let mut block = BasicBlock::new("entry".to_string());
        let ret_inst = InstructionBuilder::new(Opcode::Ret)
            .result_type(Type::Primitive(PrimitiveType::I32))
            .operand(Operand::Immediate(Value::I32(0)))
            .build();
        block.add_instruction(ret_inst);
        function.add_basic_block(block);
        
        module.add_function(function).unwrap();
        
        // Module harusnya valid
        let result = validate_module(module);
        assert!(result.is_ok());
    }

    #[test]
    fn test_invalid_module_empty_name() {
        let module = Module::new("".to_string());
        
        let result = validate_module(module);
        assert!(result.is_err());
    }

    #[test]
    fn test_invalid_function_no_blocks() {
        let mut module = Module::new("test".to_string());
        
        let func_type = FunctionType {
            return_type: Box::new(Type::Primitive(PrimitiveType::I32)),
            parameter_types: vec![],
            is_vararg: false,
        };
        
        let function = Function::new("main".to_string(), func_type);
        module.add_function(function).unwrap();
        
        let result = validate_module(module);
        assert!(result.is_err());
    }
}