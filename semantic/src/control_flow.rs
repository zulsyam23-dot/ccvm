//! Control flow analysis untuk semantic analysis
//! 
//! Modul ini bertanggung jawab untuk:
//! - Membangun graph control flow (CFG)
//! - Deteksi unreachable code
//! - Validasi terminasi fungsi
//! - Analisis dominator dan post-dominator
//! - Deteksi loops dan irreducible control flow

use crate::{SemanticError, SemanticContext, SemanticResult};
use std::collections::{HashMap, HashSet, VecDeque};
use petgraph::graph::{DiGraph, NodeIndex};
use petgraph::algo::dominators;

/// Representasi basic block dalam control flow graph
#[derive(Debug, Clone)]
pub struct BasicBlock {
    pub id: usize,
    pub instructions: Vec<Instruction>,
    pub predecessors: HashSet<usize>,
    pub successors: HashSet<usize>,
    pub is_entry: bool,
    pub is_exit: bool,
    pub is_loop_header: bool,
}

/// Representasi instruction untuk analisis control flow
#[derive(Debug, Clone)]
pub enum Instruction {
    /// Branch conditional: if condition then target1 else target2
    Branch {
        condition: String,
        true_target: usize,
        false_target: usize,
    },
    /// Unconditional jump ke target
    Jump(usize),
    /// Return dari fungsi
    Return(Option<String>),
    /// Call fungsi
    Call {
        func: String,
        args: Vec<String>,
    },
    /// Instruction biasa (tidak mengubah control flow)
    Normal,
}

/// Control Flow Graph untuk fungsi
#[derive(Debug)]
pub struct ControlFlowGraph {
    pub blocks: HashMap<usize, BasicBlock>,
    pub entry_block: usize,
    pub exit_blocks: HashSet<usize>,
    graph: DiGraph<usize, ()>,
    node_map: HashMap<usize, NodeIndex>,
}

impl ControlFlowGraph {
    pub fn new() -> Self {
        Self {
            blocks: HashMap::new(),
            entry_block: 0,
            exit_blocks: HashSet::new(),
            graph: DiGraph::new(),
            node_map: HashMap::new(),
        }
    }

    /// Tambahkan basic block ke CFG
    pub fn add_block(&mut self, block: BasicBlock) {
        let block_id = block.id;
        
        if block.is_entry {
            self.entry_block = block_id;
        }
        
        if block.is_exit {
            self.exit_blocks.insert(block_id);
        }
        
        let node_index = self.graph.add_node(block_id);
        self.node_map.insert(block_id, node_index);
        self.blocks.insert(block_id, block);
    }

    /// Bangun graph structure dari basic blocks
    pub fn build_graph(&mut self) {
        // Clear existing edges
        self.graph.clear_edges();
        
        // Tambahkan edges berdasarkan successors
        for (block_id, block) in &self.blocks {
            for &successor in &block.successors {
                if let (Some(&source_idx), Some(&target_idx)) = 
                    (self.node_map.get(block_id), self.node_map.get(&successor)) {
                    self.graph.add_edge(source_idx, target_idx, ());
                }
            }
        }
    }

    /// Cek apakah block reachable dari entry
    pub fn is_reachable(&self, block_id: usize) -> bool {
        if block_id == self.entry_block {
            return true;
        }

        let mut visited = HashSet::new();
        let mut queue = VecDeque::new();
        queue.push_back(self.entry_block);
        visited.insert(self.entry_block);

        while let Some(current) = queue.pop_front() {
            if current == block_id {
                return true;
            }
            
            if let Some(block) = self.blocks.get(&current) {
                for &successor in &block.successors {
                    if visited.insert(successor) {
                        queue.push_back(successor);
                    }
                }
            }
        }

        false
    }

    /// Dapatkan semua unreachable blocks
    pub fn get_unreachable_blocks(&self) -> HashSet<usize> {
        let mut unreachable = HashSet::new();
        
        for &block_id in self.blocks.keys() {
            if !self.is_reachable(block_id) {
                unreachable.insert(block_id);
            }
        }
        
        unreachable
    }

    /// Analisis dominator tree
    pub fn analyze_dominators(&self) -> Option<dominators::Dominators<NodeIndex>> {
        if let Some(&entry_idx) = self.node_map.get(&self.entry_block) {
            Some(dominators::simple_fast(&self.graph, entry_idx))
        } else {
            None
        }
    }

    /// Deteksi loops menggunakan dominator analysis
    pub fn detect_loops(&self) -> Vec<LoopInfo> {
        let mut loops = Vec::new();
        
        if let Some(dominators) = self.analyze_dominators() {
            for edge in self.graph.edge_indices() {
                if let Some((source, target)) = self.graph.edge_endpoints(edge) {
                    let source_id = self.graph[source];
                    let target_id = self.graph[target];
                    
                    // Back edge: source dominates target dan target reachable dari source
                    let dominates = if let Some(mut it) = dominators.dominators(target) {
                        it.any(|d| d == source)
                    } else {
                        false
                    };

                    if dominates && 
                       self.is_reachable(target_id) &&
                       source_id != target_id {
                        
                        loops.push(LoopInfo {
                            header: target_id,
                            back_edge_source: source_id,
                            blocks: self.collect_loop_blocks(target_id, source_id),
                        });
                    }
                }
            }
        }
        
        loops
    }

    /// Kumpulkan semua blocks dalam loop
    fn collect_loop_blocks(&self, header: usize, back_edge_source: usize) -> HashSet<usize> {
        let mut loop_blocks = HashSet::new();
        let mut stack = vec![back_edge_source];
        
        loop_blocks.insert(header);
        
        while let Some(current) = stack.pop() {
            if loop_blocks.insert(current) {
                // Tambahkan predecessors yang bisa mencapai header
                if let Some(block) = self.blocks.get(&current) {
                    for &pred in &block.predecessors {
                        if self.can_reach_header(pred, header, &loop_blocks) {
                            stack.push(pred);
                        }
                    }
                }
            }
        }
        
        loop_blocks
    }

    /// Cek apakah block bisa mencapai header tanpa melalui loop blocks
    fn can_reach_header(&self, start: usize, header: usize, loop_blocks: &HashSet<usize>) -> bool {
        if start == header {
            return true;
        }
        
        let mut visited = HashSet::new();
        let mut stack = vec![start];
        
        while let Some(current) = stack.pop() {
            if current == header {
                return true;
            }
            
            if !visited.insert(current) || loop_blocks.contains(&current) {
                continue;
            }
            
            if let Some(block) = self.blocks.get(&current) {
                for &pred in &block.predecessors {
                    stack.push(pred);
                }
            }
        }
        
        false
    }

    /// Validasi bahwa semua fungsi mengandung return statement
    pub fn validate_function_termination(&self) -> SemanticResult<()> {
        if self.blocks.is_empty() {
            return Err(SemanticError::ControlFlow(
                "Function contains no basic blocks".to_string()
            ));
        }

        // Cek apakah ada path dari entry ke exit
        if !self.has_path_to_exit(self.entry_block, &mut HashSet::new()) {
            return Err(SemanticError::ControlFlow(
                "Function does not terminate on all paths".to_string()
            ));
        }

        Ok(())
    }

    /// Rekursif cek apakah ada path ke exit
    fn has_path_to_exit(&self, current: usize, visited: &mut HashSet<usize>) -> bool {
        if self.exit_blocks.contains(&current) {
            return true;
        }

        if !visited.insert(current) {
            return false; // Avoid infinite recursion
        }

        if let Some(block) = self.blocks.get(&current) {
            // Cek apakah block ini mengandung return
            for instruction in &block.instructions {
                if matches!(instruction, Instruction::Return(_)) {
                    return true;
                }
            }

            // Rekursif cek successors
            for &successor in &block.successors {
                if self.has_path_to_exit(successor, visited) {
                    return true;
                }
            }
        }

        false
    }
}

/// Informasi tentang loop yang terdeteksi
#[derive(Debug, Clone)]
pub struct LoopInfo {
    pub header: usize,
    pub back_edge_source: usize,
    pub blocks: HashSet<usize>,
}

/// Control Flow Analyzer
pub struct ControlFlowAnalyzer {
    context: SemanticContext,
}

impl ControlFlowAnalyzer {
    pub fn new(context: SemanticContext) -> Self {
        Self { context }
    }

    /// Analisis control flow untuk fungsi
    pub fn analyze_function(&self, func_name: &str) -> SemanticResult<ControlFlowGraph> {
        // TODO: Integrasi dengan IR system yang sudah ada
        // Untuk sekarang, buat CFG dummy untuk demonstrasi
        
        let mut cfg = ControlFlowGraph::new();
        
        // Entry block
        let entry_block = BasicBlock {
            id: 0,
            instructions: vec![Instruction::Normal],
            predecessors: HashSet::new(),
            successors: vec![1].into_iter().collect(),
            is_entry: true,
            is_exit: false,
            is_loop_header: false,
        };
        cfg.add_block(entry_block);
        
        // Middle block dengan branch
        let middle_block = BasicBlock {
            id: 1,
            instructions: vec![
                Instruction::Branch {
                    condition: "x > 0".to_string(),
                    true_target: 2,
                    false_target: 3,
                }
            ],
            predecessors: vec![0].into_iter().collect(),
            successors: vec![2, 3].into_iter().collect(),
            is_entry: false,
            is_exit: false,
            is_loop_header: false,
        };
        cfg.add_block(middle_block);
        
        // True branch
        let true_block = BasicBlock {
            id: 2,
            instructions: vec![Instruction::Return(Some("result".to_string()))],
            predecessors: vec![1].into_iter().collect(),
            successors: HashSet::new(),
            is_entry: false,
            is_exit: true,
            is_loop_header: false,
        };
        cfg.add_block(true_block);
        cfg.exit_blocks.insert(2);
        
        // False branch
        let false_block = BasicBlock {
            id: 3,
            instructions: vec![Instruction::Return(None)],
            predecessors: vec![1].into_iter().collect(),
            successors: HashSet::new(),
            is_entry: false,
            is_exit: true,
            is_loop_header: false,
        };
        cfg.add_block(false_block);
        cfg.exit_blocks.insert(3);
        
        cfg.build_graph();
        
        // Validasi control flow
        cfg.validate_function_termination()?;
        
        // Deteksi unreachable code
        let unreachable = cfg.get_unreachable_blocks();
        if !unreachable.is_empty() {
            return Err(SemanticError::ControlFlow(format!(
                "Function '{}' contains unreachable blocks: {:?}",
                func_name, unreachable
            )));
        }
        
        // Deteksi loops
        let loops = cfg.detect_loops();
        for loop_info in &loops {
            // TODO: Validasi loop (nested loops, irreducible control flow)
            println!("Detected loop: header={}, blocks={:?}", 
                     loop_info.header, loop_info.blocks);
        }
        
        Ok(cfg)
    }

    /// Validasi control flow untuk seluruh program
    pub fn validate_program(&self) -> SemanticResult<()> {
        // TODO: Implementasi validasi program-level
        // - Cek main function
        // - Validasi semua fungsi
        // - Cek recursive calls
        
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::SemanticContext;

    #[test]
    fn test_basic_cfg_construction() {
        let mut cfg = ControlFlowGraph::new();
        
        let block1 = BasicBlock {
            id: 1,
            instructions: vec![Instruction::Normal],
            predecessors: HashSet::new(),
            successors: vec![2].into_iter().collect(),
            is_entry: true,
            is_exit: false,
            is_loop_header: false,
        };
        
        let block2 = BasicBlock {
            id: 2,
            instructions: vec![Instruction::Return(None)],
            predecessors: vec![1].into_iter().collect(),
            successors: HashSet::new(),
            is_entry: false,
            is_exit: true,
            is_loop_header: false,
        };
        
        cfg.add_block(block1);
        cfg.add_block(block2);
        cfg.build_graph();
        
        assert!(cfg.is_reachable(1));
        assert!(cfg.is_reachable(2));
        assert!(cfg.get_unreachable_blocks().is_empty());
    }

    #[test]
    fn test_unreachable_code_detection() {
        let mut cfg = ControlFlowGraph::new();
        
        // Entry block
        let entry = BasicBlock {
            id: 0,
            instructions: vec![Instruction::Jump(1)],
            predecessors: HashSet::new(),
            successors: vec![1].into_iter().collect(),
            is_entry: true,
            is_exit: false,
            is_loop_header: false,
        };
        
        // Reachable block
        let reachable = BasicBlock {
            id: 1,
            instructions: vec![Instruction::Return(None)],
            predecessors: vec![0].into_iter().collect(),
            successors: HashSet::new(),
            is_entry: false,
            is_exit: true,
            is_loop_header: false,
        };
        
        // Unreachable block
        let unreachable = BasicBlock {
            id: 2,
            instructions: vec![Instruction::Return(None)],
            predecessors: HashSet::new(),
            successors: HashSet::new(),
            is_entry: false,
            is_exit: true,
            is_loop_header: false,
        };
        
        cfg.add_block(entry);
        cfg.add_block(reachable);
        cfg.add_block(unreachable);
        cfg.build_graph();
        
        let unreachable_blocks = cfg.get_unreachable_blocks();
        assert_eq!(unreachable_blocks.len(), 1);
        assert!(unreachable_blocks.contains(&2));
    }
}