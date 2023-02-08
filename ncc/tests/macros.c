#define FOO()
#define BAR(x) x

#define BIF 1
#undef BIF

void main()
{
    // Macro with zero arguments
    FOO();

    // Macro with one argument
    BAR(1);

    // Regression: closing parens inside a string
    //BAR(")");
}
