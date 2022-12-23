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
#[derive(Copy, Clone, Debug)]
pub enum BinOp
{
    And,
    Or,
    Xor,

    Add,
    Sub,
    Mul,
    Div,
    Mod,

    Eq,
    Ne,
    Lt,
    Gt,
}

/// Expression
pub enum Expr
{
    Int(i128),
    String(String),

    Ident {
        name: String
    },

    Unary {
        op: UnOp,
        child: Box<Expr>,
    },

    Binary {
        op: BinOp,
        lhs: Box<Expr>,
        rhs: Box<Expr>,
    },

    Call {
        callee: Box<Expr>,
        args: Vec<Expr>,
    }
}

/// Statement
pub enum Stmt
{
    Expr(Expr),

    Return(Box<Expr>),

    Block(Vec<Stmt>),

    If {
        test: Expr,
        then_stmt: Box<Stmt>,
        else_stmt: Box<Stmt>,
    },

    While {
        test: Expr,
        body: Box<Stmt>,
    },

    DeclStmt {
    }
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
