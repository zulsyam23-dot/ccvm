//! Optimization passes untuk CCVM IR

use crate::{module::*, instruction::*, Result};
use std::collections::HashSet;

/// Trait untuk optimization pass
pub trait Pass {
    /// Nama pass
    fn name(&self) -> &str;
    
    /// Jalankan pass pada module
    fn run(&mut self, module: &mut Module) -> Result<bool>;
    
    /// Apakah pass ini analysis-only?
    fn is_analysis(&self) -> bool {
        false
    }
}

/// Pass manager untuk mengelola multiple passes
pub struct PassManager {
    passes: Vec<Box<dyn Pass>>,
}

impl PassManager {
    /// Buat pass manager baru
    pub fn new() -> Self {
        PassManager {
            passes: Vec::new(),
        }
    }

    /// Tambahkan pass
    pub fn add_pass<P: Pass + 'static>(&mut self, pass: P) {
        self.passes.push(Box::new(pass));
    }

    /// Jalankan semua passes
    pub fn run(&mut self, module: &mut Module) -> Result<bool> {
        let mut modified = false;
        
        for pass in &mut self.passes {
            let pass_modified = pass.run(module)?;
            modified = modified || pass_modified;
        }

        Ok(modified)
    }

    /// Ambil daftar passes
    pub fn passes(&self) -> &[Box<dyn Pass>] {
        &self.passes
    }
}

impl Default for PassManager {
    fn default() -> Self {
        Self::new()
    }
}

/// Mem2Reg Pass: Promote memory references to register references
pub struct Mem2RegPass;

impl Pass for Mem2RegPass {
    fn name(&self) -> &str {
        "mem2reg"
    }

    fn run(&mut self, module: &mut Module) -> Result<bool> {
        let mut modified = false;
        
        for function in module.functions.values_mut() {
            if self.promote_function(function)? {
                modified = true;
            }
        }

        Ok(modified)
    }
}

impl Mem2RegPass {
    fn promote_function(&self, function: &mut Function) -> Result<bool> {
        let mut modified = false;
        let mut allocas_to_remove = Vec::new();

        // Analisis alloca instructions
        for (block_idx, block) in function.basic_blocks.iter().enumerate() {
            for (inst_idx, instruction) in block.instructions.iter().enumerate() {
                if let Opcode::Alloca = instruction.opcode {
                    if self.can_promote_alloca(function, block_idx, inst_idx) {
                        allocas_to_remove.push((block_idx, inst_idx));
                    }
                }
            }
        }

        // Promote allocas yang memenuhi syarat
        for (block_idx, inst_idx) in allocas_to_remove {
            if self.promote_alloca(function, block_idx, inst_idx)? {
                modified = true;
            }
        }

        Ok(modified)
    }

    fn can_promote_alloca(&self, _function: &Function, _block_idx: usize, _inst_idx: usize) -> bool {
        // TODO: Implementasi analisis apakah alloca dapat dipromosikan
        // Sederhana: promote jika hanya digunakan untuk load/store sederhana
        true
    }

    fn promote_alloca(&self, _function: &mut Function, _block_idx: usize, _inst_idx: usize) -> Result<bool> {
        // TODO: Implementasi promosi alloca ke SSA register
        // Ini adalah placeholder untuk implementasi yang sebenarnya
        Ok(false)
    }
}

/// Dead Code Elimination Pass
pub struct DCEPass;

impl Pass for DCEPass {
    fn name(&self) -> &str {
        "dce"
    }

    fn run(&mut self, module: &mut Module) -> Result<bool> {
        let mut modified = false;

        for function in module.functions.values_mut() {
            if self.eliminate_dead_code(function)? {
                modified = true;
            }
        }

        Ok(modified)
    }
}

impl DCEPass {
    fn eliminate_dead_code(&self, function: &mut Function) -> Result<bool> {
        let mut modified = false;
        let mut live_instructions = HashSet::new();

        // Mark live instructions
        self.mark_live_instructions(function, &mut live_instructions)?;

        // Remove dead instructions
        for block in &mut function.basic_blocks {
            let original_count = block.instructions.len();
            block.instructions.retain(|inst| {
                if self.is_instruction_live(inst, &live_instructions) {
                    true
                } else {
                    modified = true;
                    false
                }
            });

            if original_count != block.instructions.len() {
                modified = true;
            }
        }

        Ok(modified)
    }

    fn mark_live_instructions(&self, function: &Function, live: &mut HashSet<String>) -> Result<()> {
        // Worklist algorithm untuk mark live instructions
        let mut worklist = Vec::new();

        // Mark all terminator instructions as live
        for block in &function.basic_blocks {
            if let Some(terminator) = block.instructions.last() {
                if let Some(reg) = self.get_instruction_result(terminator) {
                    worklist.push(reg.clone());
                    live.insert(reg);
                }
            }
        }

        // Mark instructions that produce live values
        while let Some(live_reg) = worklist.pop() {
            for block in &function.basic_blocks {
                for instruction in &block.instructions {
                    if self.instruction_uses_register(instruction, &live_reg) {
                        if let Some(result_reg) = self.get_instruction_result(instruction) {
                            if !live.contains(&result_reg) {
                                live.insert(result_reg.clone());
                                worklist.push(result_reg);
                            }
                        }
                    }
                }
            }
        }

        Ok(())
    }

    fn is_instruction_live(&self, instruction: &Instruction, live: &HashSet<String>) -> bool {
        // Terminator instructions selalu live
        if instruction.is_terminator() {
            return true;
        }

        // Instructions dengan side effects selalu live
        if self.has_side_effects(instruction) {
            return true;
        }

        // Instructions yang menghasilkan live values adalah live
        if let Some(result_reg) = self.get_instruction_result(instruction) {
            return live.contains(&result_reg);
        }

        false
    }

    fn has_side_effects(&self, instruction: &Instruction) -> bool {
        matches!(instruction.opcode,
            Opcode::Store | Opcode::Call | Opcode::AtomicCmpXchg | 
            Opcode::AtomicRMW | Opcode::Fence)
    }

    fn get_instruction_result(&self, _instruction: &Instruction) -> Option<String> {
        // TODO: Implementasi ekstraksi register name dari instruction
        // Placeholder implementation
        None
    }

    fn instruction_uses_register(&self, instruction: &Instruction, register: &str) -> bool {
        instruction.operands.iter().any(|op| {
            matches!(op, Operand::Register(reg) if reg == register)
        })
    }
}

/// Constant Folding Pass
pub struct ConstantFoldingPass;

impl Pass for ConstantFoldingPass {
    fn name(&self) -> &str {
        "constfold"
    }

    fn run(&mut self, module: &mut Module) -> Result<bool> {
        let mut modified = false;

        for function in module.functions.values_mut() {
            if self.fold_constants(function)? {
                modified = true;
            }
        }

        Ok(modified)
    }
}

impl ConstantFoldingPass {
    fn fold_constants(&self, function: &mut Function) -> Result<bool> {
        let mut modified = false;

        for block in &mut function.basic_blocks {
            for instruction in &mut block.instructions {
                if self.fold_instruction(instruction)? {
                    modified = true;
                }
            }
        }

        Ok(modified)
    }

    fn fold_instruction(&self, instruction: &mut Instruction) -> Result<bool> {
        match instruction.opcode {
            Opcode::Add | Opcode::Sub | Opcode::Mul => {
                self.fold_arithmetic(instruction)
            },
            Opcode::And | Opcode::Or | Opcode::Xor => {
                self.fold_logical(instruction)
            },
            _ => Ok(false)
        }
    }

    fn fold_arithmetic(&self, _instruction: &mut Instruction) -> Result<bool> {
        // TODO: Implementasi constant folding untuk arithmetic operations
        // Placeholder implementation
        Ok(false)
    }

    fn fold_logical(&self, _instruction: &mut Instruction) -> Result<bool> {
        // TODO: Implementasi constant folding untuk logical operations
        // Placeholder implementation
        Ok(false)
    }
}

/// Function Inlining Pass
pub struct InlinePass {
    threshold: usize, // Maximum instruction count untuk inlining
}

impl InlinePass {
    pub fn new(threshold: usize) -> Self {
        InlinePass { threshold }
    }
}

impl Pass for InlinePass {
    fn name(&self) -> &str {
        "inline"
    }

    fn run(&mut self, module: &mut Module) -> Result<bool> {
        let mut modified = false;
        let mut functions_to_inline = Vec::new();

        // Identifikasi functions yang layak untuk inlining
        for (func_name, function) in &module.functions {
            if self.should_inline(function) {
                functions_to_inline.push(func_name.clone());
            }
        }

        // Inline functions
        for func_name in functions_to_inline {
            if let Some(function) = module.functions.get(&func_name).cloned() {
                if self.inline_function(module, &func_name, &function)? {
                    modified = true;
                }
            }
        }

        Ok(modified)
    }
}

impl InlinePass {
    fn should_inline(&self, function: &Function) -> bool {
        // TODO: Implementasi heuristik untuk inlining
        // Sederhana: inline jika function kecil (kurang dari threshold instructions)
        let instruction_count = function.basic_blocks.iter()
            .map(|bb| bb.instructions.len())
            .sum::<usize>();
        
        instruction_count < self.threshold
    }

    fn inline_function(&self, _module: &mut Module, _func_name: &str, _function: &Function) -> Result<bool> {
        // TODO: Implementasi function inlining
        // Ini adalah placeholder untuk implementasi yang sebenarnya
        Ok(false)
    }
}

/// Loop Optimization Pass
pub struct LoopOptPass;

impl Pass for LoopOptPass {
    fn name(&self) -> &str {
        "loop-opt"
    }

    fn run(&mut self, module: &mut Module) -> Result<bool> {
        let mut modified = false;

        for function in module.functions.values_mut() {
            if self.optimize_loops(function)? {
                modified = true;
            }
        }

        Ok(modified)
    }
}

impl LoopOptPass {
    fn optimize_loops(&self, function: &mut Function) -> Result<bool> {
        let mut modified = false;

        // Loop invariant code motion
        if self.licm(function)? {
            modified = true;
        }

        // Loop unrolling
        if self.loop_unroll(function)? {
            modified = true;
        }

        Ok(modified)
    }

    fn licm(&self, _function: &mut Function) -> Result<bool> {
        // Loop Invariant Code Motion
        // TODO: Implementasi LICM
        Ok(false)
    }

    fn loop_unroll(&self, _function: &mut Function) -> Result<bool> {
        // Loop unrolling optimization
        // TODO: Implementasi loop unrolling
        Ok(false)
    }
}

/// Vectorization Pass
pub struct VectorizationPass;

impl Pass for VectorizationPass {
    fn name(&self) -> &str {
        "vectorize"
    }

    fn run(&mut self, module: &mut Module) -> Result<bool> {
        let mut modified = false;

        for function in module.functions.values_mut() {
            if self.vectorize_function(function)? {
                modified = true;
            }
        }

        Ok(modified)
    }
}

impl VectorizationPass {
    fn vectorize_function(&self, _function: &mut Function) -> Result<bool> {
        // TODO: Implementasi loop vectorization
        // Placeholder implementation
        Ok(false)
    }
}

/// Pass pipeline standar
pub fn create_standard_pass_pipeline() -> PassManager {
    let mut pm = PassManager::new();
    
    // Mem2Reg: Promote memory to registers
    pm.add_pass(Mem2RegPass);
    
    // Constant folding
    pm.add_pass(ConstantFoldingPass);
    
    // Dead code elimination
    pm.add_pass(DCEPass);
    
    // Function inlining
    pm.add_pass(InlinePass::new(100)); // Threshold 100 instructions
    
    // Loop optimizations
    pm.add_pass(LoopOptPass);
    
    // Vectorization
    pm.add_pass(VectorizationPass);
    
    // Final DCE untuk membersihkan sisa optimization
    pm.add_pass(DCEPass);
    
    pm
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::module::*;
    use crate::instruction::*;
    use crate::types::*;

    #[test]
    fn test_pass_manager() {
        let mut pm = PassManager::new();
        pm.add_pass(DCEPass);
        pm.add_pass(ConstantFoldingPass);
        
        assert_eq!(pm.passes().len(), 2);
    }

    #[test]
    fn test_standard_pipeline() {
        let pm = create_standard_pass_pipeline();
        assert!(pm.passes().len() > 5); // Harusnya punya banyak passes
    }
}