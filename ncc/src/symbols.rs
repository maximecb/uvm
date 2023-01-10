use std::collections::HashMap;
use crate::ast::*;
use crate::parser::{ParseError};

#[derive(Default)]
struct Scope
{
    decls: HashMap<String, Decl>,

    /// Next local variable slot index to assign
    /// this is only used for local variables
    next_idx: usize,
}

/// Represent an environment with multiple levels of scoping
#[derive(Default)]
struct Env
{
    scopes: Vec<Scope>,

    /// Number of local slots needed in the function
    num_locals: usize,
}

impl Env
{
    fn push_scope(&mut self)
    {
        let num_scopes = self.scopes.len();
        let mut new_scope = Scope::default();

        if num_scopes > 0 {
            new_scope.next_idx = self.scopes[num_scopes - 1].next_idx;
        }

        self.scopes.push(new_scope);
    }

    fn pop_scope(&mut self)
    {
        self.scopes.pop();
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

    /// Define a new variable in the topmost scope
    fn define(&mut self, name: &str, decl: Decl)
    {
        let num_scopes = self.scopes.len();
        let top_scope = &mut self.scopes[num_scopes - 1];
        assert!(top_scope.decls.get(name).is_none());

        top_scope.decls.insert(name.to_string(), decl);
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

impl Unit
{
    pub fn resolve_syms(&mut self) -> Result<(), ParseError>
    {
        let mut env = Env::default();
        env.push_scope();

        // Add definitions for all global variables
        for global in &mut self.global_vars {
            env.define(&global.name, Decl::Global {
                name: global.name.clone(),
                t: global.var_type.clone(),
            });
        }

        // Add definitions for all functions
        for fun in &mut self.fun_decls {
            env.define(&fun.name, Decl::Fun {
                name: fun.name.clone(),
                t: fun.get_type()
            });
        }

        // Resolve symbols in all functions
        for fun in &mut self.fun_decls {
            fun.resolve_syms(&mut env)?;
        }

        Ok(())
    }
}

impl Function
{
    fn resolve_syms(&mut self, env: &mut Env) -> Result<(), ParseError>
    {
        // Reset the local variable slot count
        env.num_locals = 0;

        env.push_scope();

        // Declare the function arguments
        for (idx, (param_type, param_name)) in self.params.iter().enumerate() {
            let decl = Decl::Arg { idx, t: param_type.clone() };
            env.define(param_name, decl);
        }

        self.body.resolve_syms(env)?;

        env.pop_scope();

        // Set the local variable slot count for the function
        self.num_locals = env.num_locals;

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
                init_expr.resolve_syms(env)?;
                env.define_local(var_name, var_type.clone());

                let decl = env.lookup(var_name).unwrap();
                let ref_expr = Expr::Ref(decl);

                let assign_expr = Expr::Binary {
                    op: BinOp::Assign,
                    lhs: Box::new(ref_expr),
                    rhs: Box::new(init_expr.clone()),
                };

                *self = Stmt::Expr(assign_expr);
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
            Expr::Int(_) | Expr::String(_) => {}

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

            Expr::Unary { op, child } => {
                child.as_mut().resolve_syms(env)?;
            },

            Expr::Binary { op, lhs, rhs } => {
                lhs.as_mut().resolve_syms(env)?;
                rhs.as_mut().resolve_syms(env)?;
            }

            Expr::Call { callee, args } => {
                callee.resolve_syms(env)?;
                for arg in args {
                    arg.resolve_syms(env)?;
                }
            }

            _ => todo!()
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
        use crate::parser::{Input, parse_unit};
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
    }

    #[test]
    fn globals()
    {
        parse_ok("u64 g = 5; u64 main() { return g; }");
        parse_ok("u64 g = 5; u64 main() { return g + 1; }");
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

    #[test]
    fn parse_files()
    {
        parse_file("examples/fill_rect.c");
    }
}