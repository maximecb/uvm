use crate::ast::*;
use crate::parser::{ParseError};

// FIXME: ideally, all checking should be done before we get to the
// codegen, so that it can't return an error?

impl Unit
{
    fn gen_code(&self) -> Result<String, ParseError>
    {
        let mut out: String = "".to_string();

        //
        // TODO: globals
        //

        for fun in &self.fun_decls {
            fun.gen_code(&mut out)?;
        }

        Ok((out))
    }
}

impl Function
{
    fn gen_code(&self, out: &mut String) -> Result<(), ParseError>
    {
        // Emit label for function
        out.push_str(&format!("{}:\n", self.name));








        out.push_str("\n");

        Ok(())
    }
}

impl Stmt
{
    fn gen_code(&self, out: &mut String) -> Result<(), ParseError>
    {
        Ok(())
    }
}

impl Expr
{
    fn gen_code(&self, out: &mut String) -> Result<(), ParseError>
    {
        Ok(())
    }
}

#[cfg(test)]
mod tests
{
    use super::*;




}
