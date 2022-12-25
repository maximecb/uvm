use std::collections::HashMap;
use crate::ast::*;

/// Represent an environment with multiple levels of scoping
#[derive(Default)]
struct Env
{
    scopes: Vec<HashMap<String, Decl>>,
}

impl Env
{
    fn push_scope(&mut self)
    {
        self.scopes.push(HashMap::default());
    }

    fn pop_scope(&mut self)
    {
        self.scopes.pop();
    }

    fn define(&mut self, name: &str, decl: Decl)
    {
        let num_scopes = self.scopes.len();
        let top_scope = &mut self.scopes[num_scopes - 1];
        top_scope.insert(name.to_string(), decl);
    }

    fn lookup(&self, name: &str) -> Option<Decl>
    {
        let top_idx = self.scopes.len() - 1;

        for idx in top_idx..=0 {
            let scope = &self.scopes[idx];

            if let Some(decl) = scope.get(name) {
                return Some(decl.clone());
            }
        }

        return None;
    }
}




impl Unit
{
    pub fn resolve_syms(&mut self)
    {
        let env = Env::default();







        todo!();
    }
}












#[cfg(test)]
mod tests
{
    use super::*;

    fn parse_ok(src: &str)
    {
        dbg!(src);
        //let mut input = Input::new(&src, "src");
        //parse_unit(&mut input).unwrap();
    }

    #[test]
    fn globals()
    {


    }




}
