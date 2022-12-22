pub enum Type
{
    Void,
    UInt64,
    UInt8,
}

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

/// Expression
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

/// Statement
pub enum Stmt
{
    //ExprStmt(Expr),
    Return(Box<Expr>),
}

/// Function
pub struct Function
{
    /// Name of the function
    pub name: String,

    /// Parameter list
    pub params: Vec<String>,

    /// Number of local variables
    //pub num_locals: usize,

    /// Body of the function
    pub body: Stmt,
}

/// Top-level unit (e.g. source file)
#[derive(Default)]
pub struct Unit
{
    pub fun_decls: Vec<Function>,
}
