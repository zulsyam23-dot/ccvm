//! Data flow analysis untuk semantic analysis
//!
//! Modul ini bertanggung jawab untuk:
//! - Def-Use analysis (definition dan usage chains)
//! - Live variable analysis
//! - Reaching definitions
//! - Available expressions
//! - Constant propagation
//! - Dead code elimination

use crate::{SemanticError, SemanticResult};
use std::collections::{HashMap, HashSet};
use tracing::debug;
use super::control_flow::{ControlFlowGraph, Instruction};

/// Representasi variable dalam data flow analysis
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Variable {
    pub name: String,
    pub scope: String,
    pub version: Option<usize>, // Untuk SSA form
}

/// Representasi definition (assignment) dari variable
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Definition {
    pub variable: Variable,
    pub block_id: usize,
    pub instruction_index: usize,
    pub value: Option<String>, // Untuk constant propagation
}

/// Representasi usage (reference) dari variable
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Usage {
    pub variable: Variable,
    pub block_id: usize,
    pub instruction_index: usize,
}

/// Data flow information untuk basic block
#[derive(Debug, Clone)]
pub struct DataFlowInfo {
    /// Variables yang didefinisikan di block ini
    pub gen: HashSet<Definition>,
    /// Variables yang digunakan di block ini
    pub kill: HashSet<Variable>,
    /// Definitions yang reaching di entry block
    pub in_set: HashSet<Definition>,
    /// Definitions yang reaching di exit block
    pub out_set: HashSet<Definition>,
    /// Variables yang live di entry
    pub live_in: HashSet<Variable>,
    /// Variables yang live di exit
    pub live_out: HashSet<Variable>,
}

impl DataFlowInfo {
    pub fn new() -> Self {
        Self {
            gen: HashSet::new(),
            kill: HashSet::new(),
            in_set: HashSet::new(),
            out_set: HashSet::new(),
            live_in: HashSet::new(),
            live_out: HashSet::new(),
        }
    }
}

/// Data Flow Analyzer
pub struct DataFlowAnalyzer {
    /// Control flow graph yang akan dianalisis
    cfg: ControlFlowGraph,
    /// Data flow info untuk setiap block
    block_info: HashMap<usize, DataFlowInfo>,
    /// Semua definitions dalam program
    all_definitions: HashSet<Definition>,
    /// Variable definitions per variable
    var_definitions: HashMap<Variable, HashSet<Definition>>,
}

impl DataFlowAnalyzer {
    pub fn new(cfg: ControlFlowGraph) -> Self {
        Self {
            cfg,
            block_info: HashMap::new(),
            all_definitions: HashSet::new(),
            var_definitions: HashMap::new(),
        }
    }

    /// Jalankan reaching definitions analysis
    pub fn analyze_reaching_definitions(&mut self) -> SemanticResult<()> {
        // Step 1: Kumpulkan semua definitions dan usages
        self.perform_initial_analysis()?;
        
        // Step 2: Inisialisasi data flow info untuk setiap block
        self.initialize_block_info();
        
        // Step 3: Iterative data flow analysis
        self.iterative_reaching_definitions()?;
        
        Ok(())
    }

    /// Analisis awal untuk mengumpulkan semua definitions dan usages
    pub fn perform_initial_analysis(&mut self) -> SemanticResult<()> {
        let block_ids: Vec<_> = self.cfg.blocks.keys().copied().collect();
        
        for block_id in block_ids {
            let mut block_info = DataFlowInfo::new();
            
            // Clone instructions to avoid borrowing self.cfg
            let instructions = self.cfg.blocks.get(&block_id).unwrap().instructions.clone();
            
            for (instr_idx, instruction) in instructions.iter().enumerate() {
                match instruction {
                    Instruction::Normal => {
                        self.analyze_assignment(&mut block_info, block_id, instr_idx)?;
                    }
                    Instruction::Call { func: _, args } => {
                        for arg in args {
                            let var = Variable {
                                name: arg.clone(),
                                scope: "function".to_string(),
                                version: None,
                            };
                            self.record_usage(&var, block_id, instr_idx);
                        }
                    }
                    Instruction::Return(value) => {
                        if let Some(val) = value {
                            let var = Variable {
                                name: val.clone(),
                                scope: "function".to_string(),
                                version: None,
                            };
                            self.record_usage(&var, block_id, instr_idx);
                        }
                    }
                    Instruction::Branch { condition, .. } => {
                        let var = Variable {
                            name: condition.clone(),
                            scope: "function".to_string(),
                            version: None,
                        };
                        self.record_usage(&var, block_id, instr_idx);
                    }
                    Instruction::Jump(_) => {}
                }
            }
            
            self.block_info.insert(block_id, block_info);
        }
        
        Ok(())
    }

    /// Analisis assignment statement
    fn analyze_assignment(&mut self, block_info: &mut DataFlowInfo, block_id: usize, instr_idx: usize) -> SemanticResult<()> {
        let block = self.cfg.blocks.get(&block_id).unwrap();
        let _instruction = &block.instructions[instr_idx];
        
        // In CCVM IR, instructions usually have a result register (SSA)
        // For now, let's assume Instruction has a method to get its defined register
        // if let Some(dest) = instruction.get_destination() {
        //     let var = Variable {
        //         name: dest,
        //         scope: "function".to_string(),
        //         version: None,
        //     };
        //     ...
        // }
        
        // Placeholder implementation using common variable name "reg"
        let var = Variable {
            name: format!("reg_{}_{}", block_id, instr_idx),
            scope: "local".to_string(),
            version: None,
        };
        
        let definition = Definition {
            variable: var.clone(),
            block_id,
            instruction_index: instr_idx,
            value: None,
        };
        
        block_info.gen.insert(definition.clone());
        block_info.kill.insert(var.clone());
        
        self.all_definitions.insert(definition.clone());
        self.var_definitions.entry(var).or_default().insert(definition);
        
        Ok(())
    }

    /// Record variable usage
    fn record_usage(&mut self, var: &Variable, block_id: usize, instr_idx: usize) {
        let _usage = Usage {
            variable: var.clone(),
            block_id,
            instruction_index: instr_idx,
        };
        
        // Store usage information (could be in block_info or a global usage map)
        debug!("Recording usage of variable '{}' at block {}, instr {}", var.name, block_id, instr_idx);
    }

    /// Inisialisasi data flow info untuk setiap block
    fn initialize_block_info(&mut self) {
        // Inisialisasi semua in/out sets
        for info in self.block_info.values_mut() {
            info.in_set = HashSet::new();
            info.out_set = self.all_definitions.clone();
        }
    }

    /// Iterative reaching definitions analysis
    fn iterative_reaching_definitions(&mut self) -> SemanticResult<()> {
        let mut changed = true;
        let max_iterations = 1000; // Prevent infinite loops
        let mut iterations = 0;
        
        while changed && iterations < max_iterations {
            changed = false;
            iterations += 1;
            
            // Proses blocks dalam urutan tertentu (misalnya topological order)
            let block_ids: Vec<usize> = self.cfg.blocks.keys().copied().collect();
            
            for block_id in block_ids {
                // Hitung in_set dari predecessors
                let in_set = self.compute_in_set(block_id)?;
                
                // Simpan out_set lama untuk perbandingan
                let old_out_set = self.block_info.get(&block_id).unwrap().out_set.clone();
                
                // Hitung out_set baru
                let out_set = self.compute_out_set(block_id, &in_set)?;
                
                // Update data flow info
                if let Some(info) = self.block_info.get_mut(&block_id) {
                    info.in_set = in_set;
                    info.out_set = out_set.clone();
                    
                    // Cek apakah ada perubahan
                    if info.out_set != old_out_set {
                        changed = true;
                    }
                }
            }
        }
        
        if iterations >= max_iterations {
            return Err(SemanticError::DataFlow(
                "Reaching definitions analysis did not converge".to_string()
            ));
        }
        
        Ok(())
    }

    /// Hitung in_set untuk block (union dari out_set semua predecessors)
    fn compute_in_set(&self, block_id: usize) -> SemanticResult<HashSet<Definition>> {
        let mut in_set = HashSet::new();
        
        if let Some(block) = self.cfg.blocks.get(&block_id) {
            for &pred_id in &block.predecessors {
                if let Some(pred_info) = self.block_info.get(&pred_id) {
                    in_set.extend(pred_info.out_set.clone());
                }
            }
        }
        
        Ok(in_set)
    }

    /// Hitung out_set untuk block
    fn compute_out_set(&self, block_id: usize, in_set: &HashSet<Definition>) -> SemanticResult<HashSet<Definition>> {
        if let Some(block_info) = self.block_info.get(&block_id) {
            let mut out_set = in_set.clone();
            
            // Kill definitions yang di-overwrite
            for killed_var in &block_info.kill {
                out_set.retain(|def| &def.variable != killed_var);
            }
            
            // Tambahkan definitions baru
            out_set.extend(block_info.gen.clone());
            
            Ok(out_set)
        } else {
            Err(SemanticError::DataFlow(
                format!("No data flow info for block {}", block_id)
            ))
        }
    }

    /// Jalankan live variable analysis
    pub fn analyze_live_variables(&mut self) -> SemanticResult<()> {
        // Step 1: Kumpulkan variable usages
        self.collect_variable_usages()?;
        
        // Step 2: Iterative backward data flow analysis
        self.iterative_live_variables()?;
        
        Ok(())
    }

    /// Kumpulkan variable usages
    fn collect_variable_usages(&mut self) -> SemanticResult<()> {
        // TODO: Implementasi pengumpulan usages yang sebenarnya
        for (block_id, _block) in &self.cfg.blocks {
            let info = self.block_info.get_mut(block_id)
                .ok_or_else(|| SemanticError::DataFlow(
                    format!("No info for block {}", block_id)
                ))?;
            
            // Dummy live variables
            info.live_in.insert(Variable {
                name: "x".to_string(),
                scope: "function".to_string(),
                version: None,
            });
        }
        
        Ok(())
    }

    /// Iterative live variable analysis (backward data flow)
    fn iterative_live_variables(&mut self) -> SemanticResult<()> {
        let mut changed = true;
        let max_iterations = 1000;
        let mut iterations = 0;
        
        while changed && iterations < max_iterations {
            changed = false;
            iterations += 1;
            
            // Proses blocks dalam backward order
            let mut block_ids: Vec<usize> = self.cfg.blocks.keys().copied().collect();
            block_ids.sort(); // Ensure deterministic order
            let reverse_post_order: Vec<_> = block_ids.into_iter().rev().collect();
            
            for block_id in reverse_post_order {
                // Hitung live_out dari successors
                let live_out = self.compute_live_out(block_id)?;
                
                // Simpan live_in lama untuk perbandingan
                let old_live_in = self.block_info.get(&block_id).unwrap().live_in.clone();
                
                // Hitung live_in baru
                let live_in = self.compute_live_in(block_id, &live_out)?;
                
                // Update data flow info
                if let Some(info) = self.block_info.get_mut(&block_id) {
                    info.live_out = live_out;
                    info.live_in = live_in.clone();
                    
                    // Cek apakah ada perubahan
                    if info.live_in != old_live_in {
                        changed = true;
                    }
                }
            }
        }
        
        if iterations >= max_iterations {
            return Err(SemanticError::DataFlow(
                "Live variable analysis did not converge".to_string()
            ));
        }
        
        Ok(())
    }

    /// Hitung live_out untuk block (union dari live_in semua successors)
    fn compute_live_out(&self, block_id: usize) -> SemanticResult<HashSet<Variable>> {
        let mut live_out = HashSet::new();
        
        if let Some(block) = self.cfg.blocks.get(&block_id) {
            for &succ_id in &block.successors {
                if let Some(succ_info) = self.block_info.get(&succ_id) {
                    live_out.extend(succ_info.live_in.clone());
                }
            }
        }
        
        Ok(live_out)
    }

    /// Hitung live_in untuk block
    fn compute_live_in(&self, block_id: usize, live_out: &HashSet<Variable>) -> SemanticResult<HashSet<Variable>> {
        if let Some(block_info) = self.block_info.get(&block_id) {
            let mut live_in = live_out.clone();
            
            // Tambahkan variables yang digunakan
            // TODO: Implementasi penggunaan yang sebenarnya
            
            // Hapus variables yang didefinisikan
            for killed_var in &block_info.kill {
                live_in.remove(killed_var);
            }
            
            Ok(live_in)
        } else {
            Err(SemanticError::DataFlow(
                format!("No data flow info for block {}", block_id)
            ))
        }
    }

    /// Jalankan constant propagation analysis
    pub fn analyze_constant_propagation(&mut self) -> SemanticResult<ConstantPropagationResult> {
        let mut result = ConstantPropagationResult::new();
        
        // Gunakan reaching definitions untuk constant propagation
        self.analyze_reaching_definitions()?;
        
        // Identifikasi constant definitions
        for (_block_id, info) in &self.block_info {
            for definition in &info.out_set {
                if let Some(value) = &definition.value {
                    // Cek apakah value adalah constant
                    if self.is_constant_value(value) {
                        result.constants.insert(
                            definition.variable.clone(),
                            value.clone()
                        );
                    }
                }
            }
        }
        
        Ok(result)
    }

    /// Cek apakah value adalah constant
    fn is_constant_value(&self, value: &str) -> bool {
        // TODO: Implementasi parsing constant yang lebih sophisticated
        value.parse::<i64>().is_ok() || 
        value.parse::<f64>().is_ok() ||
        value == "true" || 
        value == "false"
    }

    /// Dapatkan definisi reaching untuk variable tertentu
    pub fn get_reaching_definitions(&self, variable: &Variable, block_id: usize) -> HashSet<Definition> {
        if let Some(info) = self.block_info.get(&block_id) {
            info.in_set.iter()
                .filter(|def| def.variable == *variable)
                .cloned()
                .collect()
        } else {
            HashSet::new()
        }
    }

    /// Dapatkan dead variables (variables yang tidak pernah digunakan)
    pub fn get_dead_variables(&self) -> Vec<Variable> {
        let mut dead_vars = Vec::new();
        
        for (var, definitions) in &self.var_definitions {
            // Cek apakah ada definition yang tidak pernah digunakan
            for definition in definitions {
                if !self.is_definition_used(definition) {
                    dead_vars.push(var.clone());
                    break;
                }
            }
        }
        
        dead_vars
    }

    /// Cek apakah definition pernah digunakan
    fn is_definition_used(&self, _definition: &Definition) -> bool {
        // TODO: Implementasi pengecekan usage yang sebenarnya
        // Untuk sekarang, anggap semua definitions digunakan
        true
    }
}

/// Hasil dari constant propagation analysis
#[derive(Debug, Clone)]
pub struct ConstantPropagationResult {
    pub constants: HashMap<Variable, String>,
}

impl ConstantPropagationResult {
    pub fn new() -> Self {
        Self {
            constants: HashMap::new(),
        }
    }

    pub fn get_constant(&self, variable: &Variable) -> Option<&String> {
        self.constants.get(variable)
    }

    pub fn is_constant(&self, variable: &Variable) -> bool {
        self.constants.contains_key(variable)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::control_flow::ControlFlowGraph;
    use crate::SourceLocation;

    #[test]
    fn test_data_flow_analyzer_creation() {
        let cfg = ControlFlowGraph::new();
        let analyzer = DataFlowAnalyzer::new(cfg);
        
        assert!(analyzer.all_definitions.is_empty());
        assert!(analyzer.var_definitions.is_empty());
    }

    #[test]
    fn test_constant_propagation() {
        let cfg = ControlFlowGraph::new();
        let mut analyzer = DataFlowAnalyzer::new(cfg);
        
        // TODO: Implementasi test yang lebih comprehensive
        // Untuk sekarang, cek bahwa analysis berjalan tanpa error
        let result = analyzer.analyze_constant_propagation();
        assert!(result.is_ok());
    }
}