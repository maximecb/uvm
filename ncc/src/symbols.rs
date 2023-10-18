use std::collections::HashMap;
use crate::ast::*;
use crate::parsing::{ParseError};

#[derive(Default)]
struct Scope
{
    decls: HashMap<String, Decl>,

    /// Next local variable slot index to assign
    /// this is only used for local variables
    next_idx: usize,

    /// Current stack allocation size
    stack_alloc_size: usize,
}

/// Represent an environment with multiple levels of scoping
#[derive(Default)]
struct Env
{
    scopes: Vec<Scope>,

    /// Number of local slots needed in the function
    num_locals: usize,

    /// Local index for the stack allocation base pointer
    stack_alloc_bp: Option<usize>,

    /// Map of strings to global symbols
    string_tbl: HashMap<String, Decl>,
}

impl Env
{
    fn push_scope(&mut self)
    {
        let num_scopes = self.scopes.len();
        let mut new_scope = Scope::default();

        if num_scopes > 0 {
            new_scope.next_idx = self.scopes[num_scopes - 1].next_idx;
            new_scope.stack_alloc_size = self.scopes[num_scopes - 1].stack_alloc_size;
        }

        self.scopes.push(new_scope);
    }

    fn pop_scope(&mut self)
    {
        self.scopes.pop();
    }

    /// Define a base pointer for stack allocation
    fn define_bp(&mut self) -> usize
    {
        assert!(self.stack_alloc_bp.is_none());

        let num_scopes = self.scopes.len();
        let top_scope = &mut self.scopes[num_scopes - 1];

        let bp_idx = top_scope.next_idx;
        top_scope.next_idx += 1;
        self.stack_alloc_bp = Some(bp_idx);
        self.num_locals += 1;

        bp_idx
    }

    /// Stack allocate an object and return its offset
    fn alloc(&mut self, num_bytes: usize) -> usize
    {
        assert!(self.stack_alloc_bp.is_some());

        let num_scopes = self.scopes.len();
        let top_scope = &mut self.scopes[num_scopes - 1];

        let offset = top_scope.stack_alloc_size;
        top_scope.stack_alloc_size += num_bytes;

        offset
    }

    /// Define a new local variable in the topmost scope
    fn define_local(&mut self, name: &str, var_type: Type)
    {
        let num_scopes = self.scopes.len();
        let top_scope = &mut self.scopes[num_scopes - 1];
        assert!(top_scope.decls.get(name).is_none());

        let decl = Decl::Local {
            idx: top_scope.next_idx,
            t: var_type,
        };

        top_scope.next_idx += 1;
        if top_scope.next_idx > self.num_locals {
            self.num_locals = top_scope.next_idx;
        }

        top_scope.decls.insert(name.to_string(), decl);
    }

    /// Define a new entity in the topmost scope
    fn define(&mut self, name: &str, decl: Decl)
    {
        let num_scopes = self.scopes.len();
        let top_scope = &mut self.scopes[num_scopes - 1];

        assert!(
            top_scope.decls.get(name).is_none(),
            "two declarations with name \"{}\"",
            name
        );

        top_scope.decls.insert(name.to_string(), decl);
    }

    /// Get a global declaration for a string constant
    fn get_string(&mut self, str_const: &str) -> Decl
    {
        // Try to find the string in the string table
        if let Some(global_decl) = self.string_tbl.get(str_const) {
            return global_decl.clone();
        }

        // Generate a unique global symbol
        let sym_name = format!("__CONST_STR_{}__", self.string_tbl.len());

        // String constants are global arrays of characters
        let str_num_bytes = str_const.bytes().len() + 1;
        let new_decl = Decl::Global {
            name: sym_name.clone(),
            // FIXME: should be const char type once we support const
            t: Type::Array {
                elem_type: Box::new(Type::UInt(8)),
                size_expr: Box::new(Expr::Int(str_num_bytes as i128))
            }
        };

        self.string_tbl.insert(str_const.to_string(), new_decl.clone());
        new_decl
    }

    fn lookup(&self, name: &str) -> Option<Decl>
    {
        let top_idx = self.scopes.len() - 1;

        for idx in (0..=top_idx).rev() {

            let scope = &self.scopes[idx];

            if let Some(decl) = scope.decls.get(name) {
                return Some(decl.clone());
            }
        }

        return None;
    }
}

/// Resolve typedefs inside the AST
/// This doesn't handle potential type recursion inside structs/arrays/typedefs
fn resolve_types(t: &mut Type, env: &Env, inside_def: Option<&str>) -> Result<(), ParseError>
{
    match t {
        Type::Named(name) => {
            if let Some(inside_def) = inside_def {
                if name == inside_def {
                    return ParseError::msg_only(&format!("recursive instance of \"{}\" in typedef", name));
                }
            }

            if let Some(Decl::TypeDef { name, t: dt }) = env.lookup(name) {
                // Since we're not inside this typedef, we just clone the type
                *t = (**dt).borrow().clone();
            }
            else
            {
                return ParseError::msg_only(&format!("reference to unknown type \"{}\"", name));
            }
        }

        // If we have a pointer to a typedef inside of itself (recursive typedef),
        // then we create an Rc reference to the typedef's type to avoid infinite recursion
        Type::Pointer(t) => {
            if let Type::Named(name) = t.as_ref() {
                if let Some(Decl::TypeDef { name, t: dt }) = env.lookup(name) {
                    if let Some(inside_def) = inside_def {
                        if name == inside_def {
                            *t = Box::new(Type::Ref(dt));
                            return Ok(());
                        }
                    }
                }
                else
                {
                    return ParseError::msg_only(&format!("reference to unknown type \"{}\"", name));
                }
            }

            resolve_types(t, env, inside_def)?;
        }

        Type::Array { elem_type, size_expr } => {
            resolve_types(elem_type, env, inside_def)?;

            // TODO: process size_expr?
        }

        Type::Fun { ret_type, param_types, var_arg } => {
            resolve_types(ret_type, env, inside_def)?;

            for t in param_types {
                resolve_types(t, env, inside_def)?;
            }
        }

        Type::Struct { fields } => {
            for (name, t) in fields {
                resolve_types(t, env, inside_def)?;
            }
        }

        Type::Ref(_) => panic!(),

        _ => {}
    }

    Ok(())
}

impl Unit
{
    pub fn resolve_syms(&mut self) -> Result<(), ParseError>
    {
        let mut env = Env::default();
        env.push_scope();

        // Add definitions for each typedef
        for (name, t) in &self.typedefs {
            env.define(&name, Decl::TypeDef {
                name: name.clone(),
                t: t.clone(),
            });
        }

        // Resolve typedefs inside of typedefs
        for (name, t) in &mut self.typedefs {
            resolve_types(&mut t.borrow_mut(), &mut env, Some(name))?;
        }

        // Add definitions for all global variables
        for global in &mut self.global_vars {
            resolve_types(&mut global.var_type, &env, None)?;

            env.define(&global.name, Decl::Global {
                name: global.name.clone(),
                t: global.var_type.clone(),
            });

            // Resolve symbols in global variable initializers
            if let Some(init_expr) = &mut global.init_expr {
                init_expr.resolve_syms(&mut env)?
            }
        }

        // Add definitions for all functions
        for fun in &mut self.fun_decls {
            resolve_types(&mut fun.ret_type, &env, None)?;

            for (t, name) in &mut fun.params {
                resolve_types(t, &env, None)?;
            }

            env.define(&fun.name, Decl::Fun {
                name: fun.name.clone(),
                t: fun.get_type()
            });
        }

        // Resolve symbols in all functions
        for fun in &mut self.fun_decls {
            fun.resolve_syms(&mut env)?;
        }

        // Create new globals for each string constant
        for (str_const, decl) in env.string_tbl {
            if let Decl::Global{ name, t } = decl {
                self.global_vars.push(Global {
                    name: name.clone(),
                    var_type: t.clone(),
                    init_expr: Some(Expr::String(str_const.clone()))
                });
            }
        }

        // Sort the global variables by name so that
        // compilation is deterministic
        self.global_vars.sort_by(|a, b| a.name.cmp(&b.name));

        Ok(())
    }
}

impl Function
{
    fn resolve_syms(&mut self, env: &mut Env) -> Result<(), ParseError>
    {
        // Reset the local variable slot count
        env.num_locals = 0;
        env.stack_alloc_bp = None;

        env.push_scope();

        // Declare the function arguments
        for (idx, (param_type, param_name)) in self.params.iter().enumerate() {
            let decl = Decl::Arg { idx, t: param_type.clone() };
            env.define(param_name, decl);
        }

        // If there are stack-allocated locals in this function,
        // define a base pointer local
        self.body.each_stmt(|stmt| {
            if let Stmt::VarDecl { var_type, .. } = stmt {
                if let Type::Array { .. } = var_type {
                    if env.stack_alloc_bp.is_none() {
                        env.define_bp();
                    }
                }
            }
        });

        self.body.resolve_syms(env)?;

        env.pop_scope();

        // Set the local variable slot count for the function
        self.num_locals = env.num_locals;
        self.stack_alloc_bp = env.stack_alloc_bp;

        Ok(())
    }
}

impl Stmt
{
    fn resolve_syms(&mut self, env: &mut Env) -> Result<(), ParseError>
    {
        match self {
            Stmt::Expr(expr) => {
                expr.resolve_syms(env)?;
            }

            Stmt::Break | Stmt::Continue => {}

            Stmt::ReturnVoid => {}

            Stmt::ReturnExpr(expr) => {
                expr.resolve_syms(env)?;
            }

            Stmt::If { test_expr, then_stmt, else_stmt } => {
                test_expr.resolve_syms(env)?;
                then_stmt.resolve_syms(env)?;

                if else_stmt.is_some() {
                    else_stmt.as_mut().unwrap().resolve_syms(env)?;
                }
            }

            Stmt::While { test_expr, body_stmt } => {
                test_expr.resolve_syms(env)?;
                body_stmt.resolve_syms(env)?;
            }

            Stmt::DoWhile { test_expr, body_stmt } => {
                test_expr.resolve_syms(env)?;
                body_stmt.resolve_syms(env)?;
            }

            Stmt::For { init_stmt, test_expr, incr_expr, body_stmt } => {
                env.push_scope();

                if init_stmt.is_some() {
                    init_stmt.as_mut().unwrap().resolve_syms(env)?;
                }

                test_expr.resolve_syms(env)?;
                incr_expr.resolve_syms(env)?;

                body_stmt.resolve_syms(env)?;

                env.pop_scope();
            }

            // Local variable declaration
            Stmt::VarDecl { var_type, var_name, init_expr } => {
                resolve_types(var_type, env, None)?;

                env.define_local(var_name, var_type.clone());

                let decl = env.lookup(var_name).unwrap();
                let mut ref_expr = Expr::Ref(decl);

                // If this is an array, which will be stack-allocated
                if let Type::Array { elem_type, .. } = var_type {
                    if init_expr.is_some() {
                        return ParseError::msg_only("initialization of local array variables not yet implemented");
                    }

                    // Change the lhs ref type for the assignment
                    if let Expr::Ref(Decl::Local { ref mut t, .. }) = ref_expr {
                        *t = Type::Pointer(elem_type.clone());
                    }

                    let num_bytes = var_type.sizeof();
                    let offset = env.alloc(num_bytes);

                    // Compute the address of the array (bp - offset)
                    let bp_idx = env.stack_alloc_bp.unwrap();
                    let bp_ref = Expr::Ref(Decl::Local { idx: bp_idx, t: var_type.clone() });
                    let bp_sub = Expr::Binary {
                        op: BinOp::Add,
                        lhs: Box::new(bp_ref),
                        rhs: Box::new(Expr::Int(offset as i128))
                    };

                    let assign_expr = Expr::Binary {
                        op: BinOp::Assign,
                        lhs: Box::new(ref_expr),
                        rhs: Box::new(bp_sub),
                    };

                    *self = Stmt::Expr(assign_expr);

                    return Ok(());
                }

                // If there is an initialization expression
                if let Some(init_expr) = init_expr {
                    init_expr.resolve_syms(env)?;

                    let assign_expr = Expr::Binary {
                        op: BinOp::Assign,
                        lhs: Box::new(ref_expr),
                        rhs: Box::new(init_expr.clone()),
                    };

                    *self = Stmt::Expr(assign_expr);
                }
                else
                {
                    *self = Stmt::Expr(Expr::Int(0));
                }
            }

            Stmt::Block(stmts) => {
                env.push_scope();

                for stmt in stmts {
                    stmt.resolve_syms(env)?;
                }

                env.pop_scope();
            }
        }

        Ok(())
    }
}

impl Expr
{
    fn resolve_syms(&mut self, env: &mut Env) -> Result<(), ParseError>
    {
        match self {
            Expr::Int(_) => {}
            Expr::Float32(_) => {}

            Expr::String(str_const) => {
                // Get a global symbol for the string constant
                let decl = env.get_string(str_const);
                *self = Expr::Ref(decl);
            }

            Expr::Array(exprs) => {
                for expr in exprs {
                    expr.resolve_syms(env)?;
                }
            }

            Expr::Ident(name) => {
                //dbg!(&name);

                if let Some(decl) = env.lookup(name) {
                    *self = Expr::Ref(decl);
                }
                else
                {
                    return ParseError::msg_only(&format!("reference to undeclared identifier \"{}\"", name));
                }
            }

            Expr::Ref(_) => panic!(),

            Expr::Cast { new_type, child } => {
                if let Type::Named(name) = new_type {
                    if let Some(Decl::TypeDef { name, t }) = env.lookup(name) {
                        *new_type = (**t).borrow().clone();
                    }
                    else
                    {
                        return ParseError::msg_only(&format!("reference to unknown type \"{}\" in cast expression", name));
                    }
                }
                else
                {
                    resolve_types(new_type, env, None)?;
                }

                child.as_mut().resolve_syms(env)?;
            }

            Expr::SizeofExpr { child } => {
                child.as_mut().resolve_syms(env)?;
            }

            Expr::SizeofType { t } => {
                if let Type::Named(name) = t {
                    if let Some(Decl::TypeDef { name, t: dt }) = env.lookup(name) {
                        *t = (**dt).borrow().clone();
                    }
                    else
                    {
                        *self = Expr::SizeofExpr {
                            child: Box::new(Expr::Ident(name.clone()))
                        };

                        self.resolve_syms(env)?;
                    }
                }
                else
                {
                    resolve_types(t, env, None)?;
                }
            }

            Expr::Arrow { base, field } => {
                base.as_mut().resolve_syms(env)?;
            }

            Expr::Unary { op, child } => {
                child.as_mut().resolve_syms(env)?;
            }

            Expr::Binary { op, lhs, rhs } => {
                lhs.as_mut().resolve_syms(env)?;
                rhs.as_mut().resolve_syms(env)?;
            }

            Expr::Ternary { test_expr, then_expr, else_expr } => {
                test_expr.as_mut().resolve_syms(env)?;
                then_expr.as_mut().resolve_syms(env)?;
                else_expr.as_mut().resolve_syms(env)?;
            }

            Expr::Call { callee, args } => {
                callee.resolve_syms(env)?;
                for arg in args {
                    arg.resolve_syms(env)?;
                }
            }

            Expr::Asm { args, out_type, .. } => {
                for arg in args {
                    arg.resolve_syms(env)?;
                }

                resolve_types(out_type, env, None)?;
            }

            //_ => todo!()
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests
{
    use super::*;

    fn parse_ok(src: &str)
    {
        use crate::parsing::Input;
        use crate::parser::parse_unit;

        dbg!(src);
        let mut input = Input::new(&src, "src");
        let mut unit = parse_unit(&mut input).unwrap();
        unit.resolve_syms().unwrap();
    }

    fn parse_file(file_name: &str)
    {
        dbg!(file_name);
        let mut unit = crate::parser::parse_file(file_name).unwrap();
        unit.resolve_syms().unwrap();
    }

    #[test]
    fn basics()
    {
        parse_ok("void main() {}");
        parse_ok("void foo(u64 a) {}");
        parse_ok("u64 foo(u64 a) { return a; }");

        // Local variables
        parse_ok("void main() { u64 a = 0; }");
        parse_ok("void main(u64 a) { u64 a = 0; }");

        // Infix expressions
        parse_ok("u64 foo(u64 a, u64 b) { return a + b; }");

        // Two functions with the same parameter name
        parse_ok("void foo(u64 a) {} void bar(u64 a) {}");
    }

    #[test]
    fn globals()
    {
        parse_ok("u64 g = 5; u64 main() { return g; }");
        parse_ok("u64 g = 5; u64 main() { return g + 1; }");
        parse_ok("char* global_str = \"foo\"; void main() {}");
    }

    #[test]
    fn for_loop()
    {
        parse_ok("void main() { for (;;) {} }");
        parse_ok("void main() { for (u64 i = 0;;) {} }");
        parse_ok("void main() { for (u64 i = 0; i < 10 ;) {} }");
        parse_ok("void main() { for (u64 i = 0; i < 10 ; i = i + 1) {} }");
        parse_ok("void foo(u64 i) { for (u64 i = 0; i < 10 ; i = i + 1) {} }");
    }

    #[test]
    fn calls()
    {
        parse_ok("void foo() {} void main() { foo(); }");
    }
}
