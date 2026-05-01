//! CCVM Semantic Analyzer Library
//! 
//! Library ini menyediakan semantic analysis untuk CCVM dengan fitur:
//! - Type checking yang komprehensif
//! - Symbol resolution
#![allow(dead_code)]

//! - Control flow analysis
//! - Data flow analysis
//! - Lifetime analysis untuk Rust
//! - Borrow checking untuk Rust

use std::collections::HashMap;
use thiserror::Error;
use tracing::{debug, info};
use ccvm_core::Value;

pub mod types;
pub mod symbol_table;
pub mod control_flow;
pub mod data_flow;
pub mod lifetime;
pub mod borrow_checker;

pub use types::{TypeInfo, TypeKind, SymbolInfo, SymbolType, TypeSystem};
pub use symbol_table::*;
pub use control_flow::*;
pub use data_flow::*;
pub use lifetime::{Lifetime, BorrowKind, BorrowInfo, SourceLocation};
pub use borrow_checker::*;

/// Error types untuk semantic analyzer
#[derive(Error, Debug, Clone, PartialEq)]
pub enum SemanticError {
    #[error("Type mismatch: expected {expected}, found {found}")]
    TypeMismatch { expected: String, found: String },
    
    #[error("Undefined symbol: {name}")]
    UndefinedSymbol { name: String },
    
    #[error("Redeclaration of symbol: {name}")]
    Redeclaration { name: String },
    
    #[error("Invalid operation: {operation} on types {left_type} and {right_type}")]
    InvalidOperation { 
        operation: String, 
        left_type: String, 
        right_type: String 
    },
    
    #[error("Cannot borrow {name} as {borrow_kind}: {reason}")]
    BorrowError { 
        name: String, 
        borrow_kind: String, 
        reason: String 
    },
    
    #[error("Lifetime error: {0}")]
    Lifetime(String),
    
    #[error("Control flow error: {0}")]
    ControlFlow(String),
    
    #[error("Data flow error: {0}")]
    DataFlow(String),
    
    #[error("Borrow checker error: {0}")]
    BorrowChecker(String),
    
    #[error("Semantic error: {message}")]
    GeneralError { message: String },
}

/// Hasil operasi semantic analyzer
pub type SemanticResult<T> = std::result::Result<T, SemanticError>;

/// Alias untuk SemanticResult agar kompatibel dengan modul internal
pub type Result<T> = SemanticResult<T>;

/// Konfigurasi semantic analyzer
#[derive(Debug, Clone)]
pub struct SemanticConfig {
    pub enable_type_checking: bool,
    pub enable_borrow_checking: bool,
    pub enable_lifetime_analysis: bool,
    pub enable_control_flow_analysis: bool,
    pub enable_data_flow_analysis: bool,
    pub strict_mode: bool,
    pub language_mode: LanguageMode,
}

impl Default for SemanticConfig {
    fn default() -> Self {
        SemanticConfig {
            enable_type_checking: true,
            enable_borrow_checking: true,
            enable_lifetime_analysis: true,
            enable_control_flow_analysis: true,
            enable_data_flow_analysis: true,
            strict_mode: false,
            language_mode: LanguageMode::System,
        }
    }
}

/// Mode bahasa untuk analisis yang berbeda
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum LanguageMode {
    C,
    Cpp,
    Rust,
    Julia,
    System, // Generic system programming
}

/// Context untuk semantic analysis
pub struct SemanticContext {
    pub config: SemanticConfig,
    pub symbol_table: SymbolTable,
    pub type_system: TypeSystem,
    pub current_scope: ScopeId,
    pub current_function: Option<String>,
    pub current_module: Option<String>,
    pub errors: Vec<SemanticError>,
    pub warnings: Vec<String>,
}

impl SemanticContext {
    pub fn new(config: SemanticConfig) -> Self {
        SemanticContext {
            config,
            symbol_table: SymbolTable::new(),
            type_system: TypeSystem::new(),
            current_scope: ScopeId::global(),
            current_function: None,
            current_module: None,
            errors: Vec::new(),
            warnings: Vec::new(),
        }
    }
    
    pub fn push_error(&mut self, error: SemanticError) {
        self.errors.push(error);
    }
    
    pub fn push_warning(&mut self, warning: String) {
        self.warnings.push(warning);
    }
    
    pub fn has_errors(&self) -> bool {
        !self.errors.is_empty()
    }
    
    pub fn enter_scope(&mut self, scope_type: ScopeType) {
        self.current_scope = self.symbol_table.push_scope(scope_type);
    }
    
    pub fn exit_scope(&mut self) {
        if let Some(parent) = self.symbol_table.pop_scope() {
            self.current_scope = parent;
        }
    }
}

/// Trait untuk node yang dapat dianalisis secara semantik
pub trait SemanticAnalyzable {
    fn analyze_semantics(&self, context: &mut SemanticContext) -> SemanticResult<()>;
}

/// Trait untuk expression yang menghasilkan tipe
pub trait TypedExpression {
    fn get_type(&self, context: &SemanticContext) -> SemanticResult<TypeInfo>;
}

/// Hasil akhir dari semantic analysis
#[derive(Debug, Clone)]
pub struct SemanticAnalysisResult {
    pub symbol_table: SymbolTable,
    pub type_info: TypeSystem,
    pub errors: Vec<SemanticError>,
    pub warnings: Vec<String>,
    pub success: bool,
}

/// Representasi program untuk dianalisis
#[derive(Debug, Clone)]
pub struct Program {
    pub name: String,
    pub declarations: Vec<Declaration>,
}

#[derive(Debug, Clone)]
pub enum Declaration {
    Function(FunctionDeclaration),
    Variable(VariableDeclaration),
    Type(TypeDeclaration),
    Module(ModuleDeclaration),
}

#[derive(Debug, Clone)]
pub struct FunctionDeclaration {
    pub name: String,
    pub return_type: TypeNode,
    pub parameters: Vec<Parameter>,
    pub body: Option<Statement>,
    pub is_variadic: bool,
    pub location: SourceLocation,
}

#[derive(Debug, Clone)]
pub struct VariableDeclaration {
    pub name: String,
    pub type_info: TypeNode,
    pub initializer: Option<Expression>,
    pub is_mutable: bool,
    pub location: SourceLocation,
}

#[derive(Debug, Clone)]
pub struct TypeDeclaration {
    pub name: String,
    pub definition: TypeNode,
}

#[derive(Debug, Clone)]
pub struct ModuleDeclaration {
    pub name: String,
    pub items: Vec<Declaration>,
}

#[derive(Debug, Clone)]
pub struct Parameter {
    pub name: String,
    pub type_info: TypeNode,
}

#[derive(Debug, Clone)]
pub enum Statement {
    Block(Vec<Statement>),
    Expression(Expression),
    Return(Option<Expression>),
    If {
        condition: Expression,
        then_branch: Box<Statement>,
        else_branch: Option<Box<Statement>>,
    },
    While {
        condition: Expression,
        body: Box<Statement>,
    },
    Variable(VariableDeclaration),
}

#[derive(Debug, Clone)]
pub enum Expression {
    Literal(Value),
    Variable(String),
    Binary {
        op: String,
        left: Box<Expression>,
        right: Box<Expression>,
    },
    Unary {
        op: String,
        expr: Box<Expression>,
    },
    Call {
        func: String,
        args: Vec<Expression>,
    },
}

#[derive(Debug, Clone)]
pub enum TypeNode {
    Primitive(PrimitiveType),
    Pointer(Box<TypeNode>),
    Array { element: Box<TypeNode>, size: usize },
    Function { 
        return_type: Box<TypeNode>, 
        param_types: Vec<TypeNode>, 
        is_variadic: bool 
    },
    Struct { name: String, fields: Vec<(String, TypeNode)> },
    Named(String),
}

#[derive(Debug, Clone)]
pub enum PrimitiveType {
    Void,
    Bool,
    I8, I16, I32, I64, I128,
    F16, F32, F64, F128,
}

/// Semantic analyzer utama
pub struct SemanticAnalyzer {
    context: SemanticContext,
}

impl SemanticAnalyzer {
    pub fn new(config: SemanticConfig) -> Self {
        SemanticAnalyzer {
            context: SemanticContext::new(config),
        }
    }
    
    /// Analisis program lengkap
    pub fn analyze_program(&mut self, program: &Program) -> SemanticResult<SemanticAnalysisResult> {
        info!("Starting semantic analysis");
        
        // Pre-analysis: build symbol table
        self.build_symbol_table(program)?;
        
        // Type checking
        if self.context.config.enable_type_checking {
            self.perform_type_checking(program)?;
        }
        
        // Borrow checking (Rust specific)
        if self.context.config.enable_borrow_checking && 
           self.context.config.language_mode == LanguageMode::Rust {
            self.perform_borrow_checking(program)?;
        }
        
        // Lifetime analysis (Rust specific)
        if self.context.config.enable_lifetime_analysis && 
           self.context.config.language_mode == LanguageMode::Rust {
            self.perform_lifetime_analysis(program)?;
        }
        
        // Control flow analysis
        if self.context.config.enable_control_flow_analysis {
            self.perform_control_flow_analysis(program)?;
        }
        
        // Data flow analysis
        if self.context.config.enable_data_flow_analysis {
            self.perform_data_flow_analysis(program)?;
        }
        
        info!("Semantic analysis completed");
        
        Ok(SemanticAnalysisResult {
            symbol_table: self.context.symbol_table.clone(),
            type_info: self.context.type_system.clone(),
            errors: self.context.errors.clone(),
            warnings: self.context.warnings.clone(),
            success: !self.context.has_errors(),
        })
    }
    
    fn build_symbol_table(&mut self, program: &Program) -> SemanticResult<()> {
        debug!("Building symbol table");
        
        // We are already in global scope by default in SemanticContext::new
        
        // Process all declarations
        for declaration in &program.declarations {
            self.process_declaration(declaration)?;
        }
        
        Ok(())
    }
    
    fn process_declaration(&mut self, declaration: &Declaration) -> SemanticResult<()> {
        match declaration {
            Declaration::Function(func) => self.process_function_declaration(func),
            Declaration::Variable(var) => self.process_variable_declaration(var),
            Declaration::Type(type_decl) => self.process_type_declaration(type_decl),
            Declaration::Module(module) => self.process_module_declaration(module),
        }
    }
    
    fn process_function_declaration(&mut self, func: &FunctionDeclaration) -> SemanticResult<()> {
        let func_name = &func.name;
        
        // Check for redeclaration
        if self.context.symbol_table.lookup_current_scope(func_name).is_some() {
            return Err(SemanticError::Redeclaration {
                name: func_name.clone(),
            });
        }
        
        // Analyze return type
        let return_type = self.analyze_type(&func.return_type)?;
        
        // Analyze parameter types
        let mut param_types = Vec::new();
        for param in &func.parameters {
            let param_type = self.analyze_type(&param.type_info)?;
            param_types.push(param_type);
        }
        
        let func_type = TypeInfo {
            type_kind: TypeKind::Function {
                return_type: Box::new(return_type.clone()),
                param_types,
                is_variadic: func.is_variadic,
            },
            size: 0,
            alignment: 0,
            is_const: false,
            is_volatile: false,
            is_signed: false,
        };
        
        let symbol = SymbolInfo {
            name: func_name.clone(),
            symbol_type: SymbolType::Function,
            type_info: func_type,
            scope_level: self.context.symbol_table.current_scope_level(),
            is_mutable: false,
            is_defined: func.body.is_some(),
            location: Some(func.location.clone()),
            attributes: HashMap::new(),
        };
        
        self.context.symbol_table.insert(func_name.clone(), symbol)?;
        Ok(())
    }
    
    fn analyze_function_body(&mut self, func: &FunctionDeclaration) -> SemanticResult<()> {
        if let Some(body) = &func.body {
            self.context.current_function = Some(func.name.clone());
            
            // Enter function scope and add parameters
            self.context.enter_scope(ScopeType::Function);
            
            for param in &func.parameters {
                let param_type = self.analyze_type(&param.type_info)?;
                let symbol = SymbolInfo {
                    name: param.name.clone(),
                    symbol_type: SymbolType::Variable,
                    type_info: param_type,
                    scope_level: self.context.symbol_table.current_scope_level(),
                    is_mutable: false,
                    is_defined: true,
                    location: Some(func.location.clone()),
                    attributes: HashMap::new(),
                };
                self.context.symbol_table.insert(param.name.clone(), symbol)?;
            }
            
            self.analyze_statement(body)?;
            
            self.context.exit_scope();
            self.context.current_function = None;
        }
        Ok(())
    }
    
    fn process_variable_declaration(&mut self, var: &VariableDeclaration) -> SemanticResult<()> {
        let var_name = &var.name;
        
        // Check for redeclaration
        if self.context.symbol_table.lookup_current_scope(var_name).is_some() {
            return Err(SemanticError::Redeclaration {
                name: var_name.clone(),
            });
        }
        
        // Analyze type
        let var_type = self.analyze_type(&var.type_info)?;
        
        let symbol = SymbolInfo {
            name: var_name.clone(),
            symbol_type: SymbolType::Variable,
            type_info: var_type,
            scope_level: self.context.symbol_table.current_scope_level(),
            is_mutable: var.is_mutable,
            is_defined: var.initializer.is_some(),
            location: Some(var.location.clone()),
            attributes: HashMap::new(),
        };
        
        self.context.symbol_table.insert(var_name.clone(), symbol)?;
        Ok(())
    }
    
    fn analyze_variable_initializer(&mut self, var: &VariableDeclaration) -> SemanticResult<()> {
        if let Some(init) = &var.initializer {
            let var_symbol = self.context.symbol_table.lookup(&var.name).unwrap();
            let var_type = var_symbol.type_info.clone();
            
            let init_type = self.analyze_expression(init)?;
            self.check_type_compatibility(&var_type, &init_type)?;
        }
        Ok(())
    }
    
    fn analyze_type(&self, type_node: &TypeNode) -> SemanticResult<TypeInfo> {
        match type_node {
            TypeNode::Primitive(primitive) => self.analyze_primitive_type(primitive),
            TypeNode::Pointer(pointer) => self.analyze_pointer_type(pointer),
            TypeNode::Array { element, size } => self.analyze_array_type(element, *size),
            TypeNode::Function { return_type, param_types, is_variadic } => self.analyze_function_type_node(return_type, param_types, *is_variadic),
            TypeNode::Struct { name, fields } => self.analyze_struct_type_node(name, fields),
            TypeNode::Named(name) => self.analyze_named_type(name),
        }
    }
    
    fn analyze_primitive_type(&self, primitive: &PrimitiveType) -> SemanticResult<TypeInfo> {
        use PrimitiveType::*;
        
        let (type_kind, size, alignment) = match primitive {
            Void => (TypeKind::Void, 0, 0),
            Bool => (TypeKind::Bool, 1, 1),
            I8 => (TypeKind::Integer { bits: 8 }, 1, 1),
            I16 => (TypeKind::Integer { bits: 16 }, 2, 2),
            I32 => (TypeKind::Integer { bits: 32 }, 4, 4),
            I64 => (TypeKind::Integer { bits: 64 }, 8, 8),
            I128 => (TypeKind::Integer { bits: 128 }, 16, 16),
            F16 => (TypeKind::Float { bits: 16 }, 2, 2),
            F32 => (TypeKind::Float { bits: 32 }, 4, 4),
            F64 => (TypeKind::Float { bits: 64 }, 8, 8),
            F128 => (TypeKind::Float { bits: 128 }, 16, 16),
        };
        
        Ok(TypeInfo {
            type_kind,
            size,
            alignment,
            is_const: false,
            is_volatile: false,
            is_signed: matches!(primitive, I8 | I16 | I32 | I64 | I128),
        })
    }
    
    fn check_type_compatibility(&self, expected: &TypeInfo, found: &TypeInfo) -> SemanticResult<()> {
        if !self.are_types_compatible(expected, found) {
            return Err(SemanticError::TypeMismatch {
                expected: format_type_info(expected),
                found: format_type_info(found),
            });
        }
        Ok(())
    }
    
    fn are_types_compatible(&self, expected: &TypeInfo, found: &TypeInfo) -> bool {
        self.context.type_system.are_compatible(expected, found)
    }
    
    // Placeholder implementations for other methods
    fn analyze_expression(&mut self, expr: &Expression) -> SemanticResult<TypeInfo> {
        match expr {
            Expression::Literal(val) => self.analyze_literal(val),
            Expression::Variable(name) => {
                if let Some(symbol) = self.context.symbol_table.lookup(name) {
                    Ok(symbol.type_info.clone())
                } else {
                    let error = SemanticError::UndefinedSymbol { name: name.clone() };
                    self.context.push_error(error.clone());
                    Err(error)
                }
            },
            Expression::Binary { op, left, right } => {
                let left_type = self.analyze_expression(left)?;
                let right_type = self.analyze_expression(right)?;
                
                if self.are_types_compatible(&left_type, &right_type) {
                    // For now, assume binary op returns same type as operands
                    Ok(left_type)
                } else {
                    let error = SemanticError::InvalidOperation {
                        operation: op.clone(),
                        left_type: format_type_info(&left_type),
                        right_type: format_type_info(&right_type),
                    };
                    self.context.push_error(error.clone());
                    Err(error)
                }
            },
            Expression::Unary { op: _, expr } => {
                let expr_type = self.analyze_expression(expr)?;
                Ok(expr_type)
            },
            Expression::Call { func, args } => {
                if let Some(symbol) = self.context.symbol_table.lookup(func) {
                    if let TypeKind::Function { return_type, param_types, .. } = &symbol.type_info.type_kind {
                        if args.len() != param_types.len() {
                            let error = SemanticError::GeneralError {
                                message: format!("Function '{}' expects {} arguments, got {}", func, param_types.len(), args.len()),
                            };
                            self.context.push_error(error.clone());
                            return Err(error);
                        }
                        
                        for (i, arg) in args.iter().enumerate() {
                            let arg_type = self.analyze_expression(arg)?;
                            self.check_type_compatibility(&param_types[i], &arg_type)?;
                        }
                        
                        Ok(*return_type.clone())
                    } else {
                        let error = SemanticError::GeneralError {
                            message: format!("Symbol '{}' is not a function", func),
                        };
                        self.context.push_error(error.clone());
                        Err(error)
                    }
                } else {
                    let error = SemanticError::UndefinedSymbol { name: func.clone() };
                    self.context.push_error(error.clone());
                    Err(error)
                }
            }
        }
    }
    
    fn analyze_literal(&self, val: &Value) -> SemanticResult<TypeInfo> {
        let (type_kind, size, alignment) = match val {
            Value::I1(_) => (TypeKind::Bool, 1, 1),
            Value::I8(_) => (TypeKind::Integer { bits: 8 }, 1, 1),
            Value::I16(_) => (TypeKind::Integer { bits: 16 }, 2, 2),
            Value::I32(_) => (TypeKind::Integer { bits: 32 }, 4, 4),
            Value::I64(_) => (TypeKind::Integer { bits: 64 }, 8, 8),
            Value::I128(_) => (TypeKind::Integer { bits: 128 }, 16, 16),
            Value::F16(_) => (TypeKind::Float { bits: 16 }, 2, 2),
            Value::F32(_) => (TypeKind::Float { bits: 32 }, 4, 4),
            Value::F64(_) => (TypeKind::Float { bits: 64 }, 8, 8),
            Value::F128(_) => (TypeKind::Float { bits: 128 }, 16, 16),
            Value::Array(vals) => {
                if vals.is_empty() {
                    (TypeKind::Unknown, 0, 1)
                } else {
                    let element_type = self.analyze_literal(&vals[0])?;
                    (TypeKind::Array {
                        element: Box::new(element_type.clone()),
                        size: vals.len(),
                    }, element_type.size * vals.len(), element_type.alignment)
                }
            },
            Value::Struct(_vals) => {
                (TypeKind::Struct {
                    name: "anonymous".to_string(),
                    fields: vec![],
                }, 0, 1)
            },
            Value::Null => (TypeKind::Pointer(Box::new(TypeInfo {
                type_kind: TypeKind::Void,
                size: 0,
                alignment: 0,
                is_const: false,
                is_volatile: false,
                is_signed: false,
            })), 8, 8),
        };
        
        Ok(TypeInfo {
            type_kind,
            size,
            alignment,
            is_const: true,
            is_volatile: false,
            is_signed: true, // Default to signed
        })
    }
    
    fn analyze_statement(&mut self, stmt: &Statement) -> SemanticResult<()> {
        match stmt {
            Statement::Block(stmts) => {
                self.context.enter_scope(ScopeType::Block);
                for s in stmts {
                    self.analyze_statement(s)?;
                }
                self.context.exit_scope();
                Ok(())
            },
            Statement::Expression(expr) => {
                self.analyze_expression(expr)?;
                Ok(())
            },
            Statement::Return(expr_opt) => {
                if let Some(expr) = expr_opt {
                    let expr_type = self.analyze_expression(expr)?;
                    // Check return type compatibility if in a function
                    if let Some(func_name) = &self.context.current_function {
                        if let Some(symbol) = self.context.symbol_table.lookup(func_name) {
                            if let TypeKind::Function { return_type, .. } = &symbol.type_info.type_kind {
                                self.check_type_compatibility(return_type, &expr_type)?;
                            }
                        }
                    }
                }
                Ok(())
            },
            Statement::If { condition, then_branch, else_branch } => {
                let cond_type = self.analyze_expression(condition)?;
                if !matches!(cond_type.type_kind, TypeKind::Bool) {
                    return Err(SemanticError::TypeMismatch {
                        expected: "bool".to_string(),
                        found: format_type_info(&cond_type),
                    });
                }
                self.analyze_statement(then_branch)?;
                if let Some(else_b) = else_branch {
                    self.analyze_statement(else_b)?;
                }
                Ok(())
            },
            Statement::While { condition, body } => {
                let cond_type = self.analyze_expression(condition)?;
                if !matches!(cond_type.type_kind, TypeKind::Bool) {
                    return Err(SemanticError::TypeMismatch {
                        expected: "bool".to_string(),
                        found: format_type_info(&cond_type),
                    });
                }
                self.analyze_statement(body)?;
                Ok(())
            },
            Statement::Variable(var) => {
                self.process_variable_declaration(var)
            }
        }
    }
    
    fn perform_type_checking(&mut self, program: &Program) -> SemanticResult<()> {
        debug!("Performing type checking");
        
        for declaration in &program.declarations {
            match declaration {
                Declaration::Function(func) => self.analyze_function_body(func)?,
                Declaration::Variable(var) => self.analyze_variable_initializer(var)?,
                _ => {}
            }
        }
        Ok(())
    }
    
    fn perform_borrow_checking(&mut self, _program: &Program) -> SemanticResult<()> {
        debug!("Performing borrow checking");
        // Implementation would perform Rust-style borrow checking
        Ok(())
    }
    
    fn perform_lifetime_analysis(&mut self, _program: &Program) -> SemanticResult<()> {
        debug!("Performing lifetime analysis");
        // Implementation would perform lifetime analysis
        Ok(())
    }
    
    fn perform_control_flow_analysis(&mut self, _program: &Program) -> SemanticResult<()> {
        debug!("Performing control flow analysis");
        // Implementation would perform control flow analysis
        Ok(())
    }
    
    fn perform_data_flow_analysis(&mut self, _program: &Program) -> SemanticResult<()> {
        debug!("Performing data flow analysis");
        // Implementation would perform data flow analysis
        Ok(())
    }
    
    fn process_type_declaration(&mut self, _type_decl: &TypeDeclaration) -> SemanticResult<()> {
        // Implementation would process type declarations
        Ok(())
    }
    
    fn process_module_declaration(&mut self, _module: &ModuleDeclaration) -> SemanticResult<()> {
        // Implementation would process module declarations
        Ok(())
    }
    
    fn analyze_pointer_type(&self, pointer: &TypeNode) -> SemanticResult<TypeInfo> {
        let pointee_type = self.analyze_type(pointer)?;
        Ok(TypeInfo {
            type_kind: TypeKind::Pointer(Box::new(pointee_type)),
            size: 8,
            alignment: 8,
            is_const: false,
            is_volatile: false,
            is_signed: false,
        })
    }
    
    fn analyze_array_type(&self, element: &TypeNode, size: usize) -> SemanticResult<TypeInfo> {
        let element_type = self.analyze_type(element)?;
        Ok(TypeInfo {
            type_kind: TypeKind::Array {
                element: Box::new(element_type.clone()),
                size,
            },
            size: element_type.size * size,
            alignment: element_type.alignment,
            is_const: false,
            is_volatile: false,
            is_signed: false,
        })
    }
    
    fn analyze_function_type_node(&self, return_type: &TypeNode, param_types: &[TypeNode], is_variadic: bool) -> SemanticResult<TypeInfo> {
        let ret_type = self.analyze_type(return_type)?;
        let mut params = Vec::new();
        for p in param_types {
            params.push(self.analyze_type(p)?);
        }
        
        Ok(TypeInfo {
            type_kind: TypeKind::Function {
                return_type: Box::new(ret_type),
                param_types: params,
                is_variadic,
            },
            size: 0,
            alignment: 0,
            is_const: false,
            is_volatile: false,
            is_signed: false,
        })
    }
    
    fn analyze_struct_type_node(&self, name: &str, fields: &[(String, TypeNode)]) -> SemanticResult<TypeInfo> {
        let mut struct_fields = Vec::new();
        let mut size = 0;
        let mut alignment = 1;
        
        for (f_name, f_type) in fields {
            let field_type = self.analyze_type(f_type)?;
            size += field_type.size;
            alignment = alignment.max(field_type.alignment);
            struct_fields.push((f_name.clone(), field_type));
        }
        
        Ok(TypeInfo {
            type_kind: TypeKind::Struct {
                name: name.to_string(),
                fields: struct_fields,
            },
            size,
            alignment,
            is_const: false,
            is_volatile: false,
            is_signed: false,
        })
    }
    
    fn analyze_named_type(&self, name: &str) -> SemanticResult<TypeInfo> {
        if let Some(type_info) = self.context.type_system.get_type(name) {
            Ok(type_info)
        } else {
            let error = SemanticError::GeneralError {
                message: format!("Undefined type: {}", name),
            };
            // Can't push error here because &self
            Err(error)
        }
    }
}

/// Helper function untuk format type info
fn format_type_info(type_info: &TypeInfo) -> String {
    format!("{:?}", type_info.type_kind)
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_semantic_analyzer_creation() {
        let config = SemanticConfig::default();
        let analyzer = SemanticAnalyzer::new(config);
        
        assert!(!analyzer.context.has_errors());
        assert_eq!(analyzer.context.symbol_table.current_scope_level(), 0);
    }
    
    #[test]
    fn test_type_compatibility() {
        let config = SemanticConfig::default();
        let analyzer = SemanticAnalyzer::new(config);
        
        let type1 = TypeInfo {
            type_kind: TypeKind::Integer { bits: 32 },
            size: 4,
            alignment: 4,
            is_const: false,
            is_volatile: false,
            is_signed: true,
        };
        
        let type2 = type1.clone();
        
        assert!(analyzer.are_types_compatible(&type1, &type2));
    }

    #[test]
    fn test_analyze_program_success() {
        let mut program = Program {
            name: "test_prog".to_string(),
            declarations: vec![
                Declaration::Variable(VariableDeclaration {
                    name: "x".to_string(),
                    type_info: TypeNode::Primitive(PrimitiveType::I32),
                    initializer: Some(Expression::Literal(Value::I32(42))),
                    is_mutable: true,
                    location: SourceLocation { file: "test.ccvm".to_string(), line: 1, column: 1 },
                }),
                Declaration::Function(FunctionDeclaration {
                    name: "main".to_string(),
                    return_type: TypeNode::Primitive(PrimitiveType::I32),
                    parameters: vec![],
                    body: Some(Statement::Block(vec![
                        Statement::Return(Some(Expression::Variable("x".to_string())))
                    ])),
                    is_variadic: false,
                    location: SourceLocation { file: "test.ccvm".to_string(), line: 2, column: 1 },
                })
            ],
        };
        
        let mut analyzer = SemanticAnalyzer::new(SemanticConfig::default());
        let result = analyzer.analyze_program(&program).unwrap();
        
        assert!(result.success);
        assert!(result.errors.is_empty());
    }

    #[test]
    fn test_analyze_program_type_mismatch() {
        let mut program = Program {
            name: "test_prog".to_string(),
            declarations: vec![
                Declaration::Variable(VariableDeclaration {
                    name: "x".to_string(),
                    type_info: TypeNode::Primitive(PrimitiveType::I32),
                    initializer: Some(Expression::Literal(Value::I1(true))),
                    is_mutable: false,
                    location: SourceLocation { file: "test.ccvm".to_string(), line: 1, column: 1 },
                })
            ],
        };
        
        let mut analyzer = SemanticAnalyzer::new(SemanticConfig::default());
        let result = analyzer.analyze_program(&program);
        
        assert!(result.is_err());
    }
}
