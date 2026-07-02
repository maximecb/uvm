// Two lower-risk gaps DOOM exercises that nothing else here does:
//   * the comma operator, both as multiple init/update clauses in a for-loop
//     (for (i=0, j=n; ...; i++, j--)) and as a parenthesised expression whose
//     value is its rightmost operand;
//   * adjacent string-literal concatenation ("ab" "cd" -> one "abcd" constant),
//     a lexer feature DOOM uses to split long strings (PureDOOM.h:9882).

static int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }

int main(void)
{
    int a[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };

    // Comma in both the init and the update clause of the for-loop.
    int pairs = 0;
    for (int i = 0, j = 7; i < j; i++, j--)
        pairs += a[i] + a[j];                   // (1+8)+(2+7)+(3+6)+(4+5) = 36

    // Comma operator as an expression: evaluates left, yields the right.
    int x = (pairs, pairs + 4);                 // 40

    // Adjacent string literals are concatenated into a single constant.
    const char *s = "abc" "def" "ghi";
    int len = slen(s);                          // 9

    return x + len;                             // 40 + 9 = 49
}
