use std::collections::HashMap;
use crate::ast::*;
use crate::parsing::{ParseError};

/// Resolve typedefs inside the AST
/// This doesn't handle potential type recursion inside structs/arrays/typedefs
fn resolve_types(
    t: &mut Type,
    typedefs: &HashMap<String, TypeDef>,
    inside_def: Option<&str>
) -> Result<(), ParseError>
{
    match t {
        Type::Named(name) => {
            if let Some(inside_def) = inside_def {
                if name == inside_def {
                    return ParseError::msg_only(&format!("recursive instance of \"{}\" in typedef", name));
                }
            }

            if let Some(dt) = typedefs.get(name) {
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
                if let Some(dt) = typedefs.get(name) {
                    if let Some(inside_def) = inside_def {
                        if name == inside_def {
                            *t = Box::new(Type::Ref(dt.clone()));
                            return Ok(());
                        }
                    }
                }
                else
                {
                    return ParseError::msg_only(&format!("reference to unknown type \"{}\"", name));
                }
            }

            resolve_types(t, typedefs, inside_def)?;
        }

        Type::Array { elem_type, size_expr } => {
            resolve_types(elem_type, typedefs, inside_def)?;

            // TODO: process size_expr?
        }

        Type::Fun { ret_type, param_types, var_arg } => {
            resolve_types(ret_type, typedefs, inside_def)?;

            for t in param_types {
                resolve_types(t, typedefs, inside_def)?;
            }
        }

        Type::Struct { fields } => {
            for (name, t) in fields {
                resolve_types(t, typedefs, inside_def)?;
            }
        }

        Type::Ref(_) => panic!(),

        _ => {}
    }

    Ok(())
}

impl Unit
{
    pub fn resolve_types(&mut self) -> Result<(), ParseError>
    {
        let mut typedefs = HashMap::default();

        // Add definitions for each typedef
        for (name, t) in &self.typedefs {
            typedefs.insert(name.clone(), t.clone());
        }

        // Resolve typedefs inside of typedefs
        for (name, t) in &mut self.typedefs {
            resolve_types(&mut t.borrow_mut(), &typedefs, Some(name))?;
        }

        // Resolve global variable types
        for global in &mut self.global_vars {
            resolve_types(&mut global.var_type, &typedefs, None)?;

            // Resolve symbols in global variable initializers
            if let Some(init_expr) = &mut global.init_expr {
                init_expr.resolve_types(&typedefs)?
            }
        }

        // Resolve types in all functions
        for fun in &mut self.fun_decls {
            fun.resolve_types(&typedefs)?;
        }

        Ok(())
    }
}

impl Function
{
    fn resolve_types(&mut self, typedefs: &HashMap<String, TypeDef>) -> Result<(), ParseError>
    {
        // Resolve the argument types
        for (idx, (param_type, param_name)) in self.params.iter_mut().enumerate() {
            resolve_types(param_type, typedefs, None)?;
        }

        resolve_types(&mut self.ret_type, &typedefs, None)?;

        self.body.resolve_types(typedefs)?;

        Ok(())
    }
}

impl Stmt
{
    fn resolve_types(&mut self, typedefs: &HashMap<String, TypeDef>) -> Result<(), ParseError>
    {
        match self {
            Stmt::Expr(expr) => {
                expr.resolve_types(typedefs)?;
            }

            Stmt::Break | Stmt::Continue => {}

            Stmt::ReturnVoid => {}

            Stmt::ReturnExpr(expr) => {
                expr.resolve_types(typedefs)?;
            }

            Stmt::If { test_expr, then_stmt, else_stmt } => {
                test_expr.resolve_types(typedefs)?;
                then_stmt.resolve_types(typedefs)?;

                if else_stmt.is_some() {
                    else_stmt.as_mut().unwrap().resolve_types(typedefs)?;
                }
            }

            Stmt::While { test_expr, body_stmt } => {
                test_expr.resolve_types(typedefs)?;
                body_stmt.resolve_types(typedefs)?;
            }

            Stmt::DoWhile { test_expr, body_stmt } => {
                test_expr.resolve_types(typedefs)?;
                body_stmt.resolve_types(typedefs)?;
            }

            Stmt::For { init_stmt, test_expr, incr_expr, body_stmt } => {
                if init_stmt.is_some() {
                    init_stmt.as_mut().unwrap().resolve_types(typedefs)?;
                }

                test_expr.resolve_types(typedefs)?;
                incr_expr.resolve_types(typedefs)?;

                body_stmt.resolve_types(typedefs)?;
            }

            // Local variable declaration
            Stmt::VarDecl { var_type, var_name, init_expr } => {
                resolve_types(var_type, typedefs, None)?;
            }

            Stmt::Block(stmts) => {
                for stmt in stmts {
                    stmt.resolve_types(typedefs)?;
                }
            }
        }

        Ok(())
    }
}

impl Expr
{
    fn resolve_types(&mut self, typedefs: &HashMap<String, TypeDef>) -> Result<(), ParseError>
    {
        match self {
            Expr::Int(_) => {}
            Expr::Float32(_) => {}

            Expr::String(str_const) => {}

            Expr::Array(exprs) => {
                for expr in exprs {
                    expr.resolve_types(typedefs)?;
                }
            }

            Expr::Ident(name) => {}

            Expr::Ref(_) => panic!(),

            Expr::Cast { new_type, child } => {
                if let Type::Named(name) = new_type {
                    if let Some(t) = typedefs.get(name) {
                        *new_type = (**t).borrow().clone();
                    }
                    else
                    {
                        return ParseError::msg_only(&format!("reference to unknown type \"{}\" in cast expression", name));
                    }
                }
                else
                {
                    resolve_types(new_type, typedefs, None)?;
                }

                child.as_mut().resolve_types(typedefs)?;
            }

            Expr::SizeofExpr { child } => {
                child.as_mut().resolve_types(typedefs)?;
            }

            Expr::SizeofType { t } => {
                if let Type::Named(name) = t {
                    if let Some(dt) = typedefs.get(name) {
                        *t = (**dt).borrow().clone();
                    }
                    else
                    {
                        *self = Expr::SizeofExpr {
                            child: Box::new(Expr::Ident(name.clone()))
                        };
                    }
                }
                else
                {
                    resolve_types(t, typedefs, None)?;
                }
            }

            Expr::Arrow { base, field } => {
                base.as_mut().resolve_types(typedefs)?;
            }

            Expr::Unary { op, child } => {
                child.as_mut().resolve_types(typedefs)?;
            }

            Expr::Binary { op, lhs, rhs } => {
                lhs.as_mut().resolve_types(typedefs)?;
                rhs.as_mut().resolve_types(typedefs)?;
            }

            Expr::Ternary { test_expr, then_expr, else_expr } => {
                test_expr.as_mut().resolve_types(typedefs)?;
                then_expr.as_mut().resolve_types(typedefs)?;
                else_expr.as_mut().resolve_types(typedefs)?;
            }

            Expr::Call { callee, args } => {
                callee.resolve_types(typedefs)?;
                for arg in args {
                    arg.resolve_types(typedefs)?;
                }
            }

            Expr::Asm { args, out_type, .. } => {
                for arg in args {
                    arg.resolve_types(typedefs)?;
                }

                resolve_types(out_type, typedefs, None)?;
            }

            //_ => todo!()
        }

        Ok(())
    }
}
