/// Unary operator
pub enum UnOp
{
    Minus,
    Not,
}

/// Binary operator
pub enum BinOp
{
    Add,
    Sub,
    Mul,
    Div,
    Mod,
}

pub enum Expr
{
    Int(i128),

    Unary {
        op: UnOp,
        child: Box<Expr>,
    },

    Binary {
        op: BinOp,
        lhs: Box<Expr>,
        rhs: Box<Expr>,
    },

    //Call
}



pub enum Stmt
{
    ExprStmt(Expr),
}



pub struct Function
{

}



pub struct Unit
{

}
