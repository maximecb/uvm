// Reference:
// https://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html

use std::path::Path;
use std::collections::HashMap;
use crate::parsing::*;

impl Input
{
    /// Eat whitespace characters, but stop at newlines
    fn eat_spaces(&mut self)
    {
        loop
        {
            if self.eof() {
                break;
            }

            let ch = self.peek_ch();

            if ch == ' ' || ch == '\t' || ch == '\r' {
                self.eat_ch();
            }
            else
            {
                break;
            }
        }
    }

    /// Read a character string literal and pass it to the output as-is
    fn read_string(&mut self, close_ch: char) -> Result<String, ParseError>
    {
        let mut output = String::new();

        // Read the opening character
        output.push(self.eat_ch());

        loop
        {
            if self.eof() {
                return self.parse_error("end of input inside string");
            }

            let ch = self.eat_ch();
            output.push(ch);

            if ch == close_ch {
                break;
            }

            // Backslash character
            if ch == '\\' {
                let ch = self.eat_ch();
                output.push(ch);
                continue;
            }
        }

        Ok(output)
    }
}

#[derive(Clone, Debug)]
struct Def
{
    name: String,
    params: Option<Vec<String>>,
    text: String,
}

/// Parse a definition or macro
fn parse_def(input: &mut Input) -> Result<Def, ParseError>
{
    let name = input.parse_ident()?;

    let mut params = None;

    // If there are macro parameters
    if input.match_char('(') {
        let mut param_vec = Vec::default();

        loop
        {
            if input.match_token(")")? {
                break;
            }

            if input.eof() {
                return input.parse_error("eof inside #define macro parameters");
            }

            param_vec.push(input.parse_ident()?);

            if input.match_token(")")? {
                break;
            }

            input.expect_token(",")?;
        }

        params = Some(param_vec);
    }

    // Read text until we hit a newline \n
    let mut text = "".to_string();
    loop
    {
        if input.eof() {
            break;
        }

        let ch = input.peek_ch();

        if ch == '\n' {
            break;
        }

        // Eat single-line comments
        if input.match_chars(&['/', '/']) {
            input.eat_comment();
            text += " ";
            continue;
        }

        // Eat multi-line comments
        if input.match_chars(&['/', '*']) {
            input.eat_multi_comment()?;
            text += " ";
            continue;
        }

        // If this is a character string or character literal
        if ch == '"' || ch == '\'' {
            text += &input.read_string(ch)?;
            continue;
        }

        // Backslash to keep reading on the next line
        if ch == '\\' {
            input.eat_ch();

            loop
            {
                if input.eof() {
                    break;
                }

                match input.eat_ch() {
                    '\n' => break,
                    '\r' => {},
                    ' ' => {},
                    _ => return input.parse_error("expected newline")
                }
            }
        }

        text.push(input.eat_ch());
    }

    text = text.trim().to_string();

    Ok(Def {
        name,
        params,
        text,
    })
}

fn process_ifdef(
    input: &mut Input,
    defs: &mut HashMap<String, Def>,
    counter: &mut usize,
    gen_output: bool,
) -> Result<String, ParseError>
{
    let ident = input.parse_ident()?;
    let is_defined = defs.get(&ident).is_some();

    process_branches(
        input,
        defs,
        counter,
        gen_output,
        is_defined,
    )
}

fn process_ifndef(
    input: &mut Input,
    defs: &mut HashMap<String, Def>,
    counter: &mut usize,
    gen_output: bool,
) -> Result<String, ParseError>
{
    let ident = input.parse_ident()?;
    let is_defined = defs.get(&ident).is_some();

    process_branches(
        input,
        defs,
        counter,
        gen_output,
        !is_defined,
    )
}

/// Process conditional branches for an if-else type of directive
fn process_branches(
    input: &mut Input,
    defs: &mut HashMap<String, Def>,
    counter: &mut usize,
    gen_output: bool,
    branch_cond: bool
) -> Result<String, ParseError>
{
    let mut output = String::new();

    // If the condition is true
    if branch_cond {
        // Process the then branch normally
        let (sub_output, end_keyword) = process_input_rec(
            input,
            defs,
            counter,
            gen_output,
        )?;

        // If there is an else branch
        if end_keyword == "else" {
            let (_, end_keyword) = process_input_rec(
                input,
                defs,
                counter,
                false,
            )?;

            if end_keyword != "endif" {
                return input.parse_error("expected #endif");
            }
        }

        output += &sub_output;
    }
    else
    {
        // Name defined, we need to ignore the then branch
        let (_, end_keyword) = process_input_rec(
            input,
            defs,
            counter,
            false,
        )?;

        // If there is an else branch
        if end_keyword == "else" {
            let (sub_output, end_keyword) = process_input_rec(
                input,
                defs,
                counter,
                gen_output,
            )?;

            if end_keyword != "endif" {
                return input.parse_error("expected #endif");
            }

            output += &sub_output;
        }
    }

    Ok(output)
}

// Read a macro argument
fn read_macro_arg(input: &mut Input, depth: usize) -> Result<String, ParseError>
{
    let mut output = "".to_string();

    loop
    {
        if input.eof() {
            return input.parse_error("end of input inside macro argument");
        }

        let ch = input.peek_ch();

        // If this is a character string or character literal
        if ch == '"' || ch == '\'' {
            output += &input.read_string(ch)?;
            continue;
        }

        // If this is an opening parenthesis
        if ch == '(' {
            input.eat_ch();
            output.push('(');
            output += &read_macro_arg(input, depth + 1)?;
            input.eat_ch();
            output.push(')');
            continue;
        }

        if ch == ')' {
            break;
        }

        if ch == ',' {
            if depth == 0 {
                break;
            }
        }

        output.push(input.eat_ch());
    }

    Ok(output)
}

/// Expand a definition or macro
fn expand_macro(
    input: &mut Input,
    defs: &mut HashMap<String, Def>,
    counter: &mut usize,
    gen_output: bool,
    def: &Def,
) -> Result<String, ParseError>
{
    let mut text = def.text.clone();

    // If this is a macro with arguments
    if let Some(params) = &def.params {
        // If no arguments are provided,
        // don't expand the definition
        if !input.match_token("(")? {
            return Ok(def.name.clone());
        }

        let mut args = Vec::new();

        // For each macro argument
        loop
        {
            if input.eof() {
                return input.parse_error("unexpected end of input");
            }

            if input.match_token(")")? {
                break;
            }

            args.push(read_macro_arg(input, 0)?);

            if input.match_token(")")? {
                break;
            }

            input.expect_token(",")?;
        }

        // If the argument count doesn't match
        if args.len() != params.len() {
            return input.parse_error(&format!(
                "macro {} expected {} arguments",
                def.name,
                params.len()
            ));
        }

        // Map parameter names to argument values
        let mut param_to_arg = HashMap::new();
        for (idx, param) in params.iter().enumerate() {
            param_to_arg.insert(param, &args[idx]);
        }

        // TODO: use the src name/loc from the macro definition
        let mut input = Input::new(&text, &input.src_name);
        let mut output = String::new();

        // Loop through the macro text
        loop
        {
            if input.eof() {
                break;
            }

            let ch = input.peek_ch();

            // If this is a character string or character literal
            if ch == '"' || ch == '\'' {
                output += &input.read_string(ch)?;
                continue;
            }

            // Eat single-line comments
            if input.match_chars(&['/', '/']) {
                input.eat_comment();
                output += " ";
                continue;
            }

            // Eat multi-line comments
            if input.match_chars(&['/', '*']) {
                input.eat_multi_comment()?;
                output += " ";
                continue;
            }

            // If this is an identifier
            if is_ident_start(ch) {
                let ident = input.parse_ident()?;

                // If we have a definition for this identifier
                if let Some(arg_val) = param_to_arg.get(&ident) {
                    output += arg_val;
                } else {
                    output += &ident;
                }

                continue;
            }

            output.push(input.eat_ch());
        }

        text = output;
    }

    // Process macros in text recursively
    let mut input = Input::new(&text, &input.src_name);
    let (sub_input, end_keyword) = process_input_rec(
        &mut input,
        defs,
        counter,
        gen_output,
    )?;

    if end_keyword != "" {
        return input.parse_error(&format!("unexpected #{}", end_keyword));
    }

    return Ok(sub_input);
}

/// Process the input and generate an output string
pub fn process_input(input: &mut Input) -> Result<String, ParseError>
{
    process_input_with_defs(input, &HashMap::new())
}

/// Process the input and generate an output string,
/// Given a map of input definitions
pub fn process_input_with_defs(
    input: &mut Input,
    in_defs: &HashMap<String, String>,
) -> Result<String, ParseError>
{
    // Counter variable accessible through preprocessing
    let mut counter = 0;

    // Create a map of definitions/macros
    let mut defs = HashMap::new();
    for (name, value) in in_defs {
        defs.insert(
            name.clone(),
            Def {
                name: name.clone(),
                params: None,
                text: value.clone(),
            }
        );
    }

    let (output, end_keyword) = process_input_rec(
        input,
        &mut defs,
        &mut counter,
        true,
    )?;

    if end_keyword != "" {
        return input.parse_error(&format!("unexpected #{}", end_keyword));
    }

    Ok(output)
}

/// Process the input and generate an output string recursively
fn process_input_rec(
    input: &mut Input,
    defs: &mut HashMap<String, Def>,
    counter: &mut usize,
    gen_output: bool,
) -> Result<(String, String), ParseError>
{
    let mut output = String::new();

    // For each line of the input
    loop
    {
        if input.eof() {
            break;
        }

        let ch = input.peek_ch();

        // If this is a preprocessor directive
        if input.peek_ch() == '#' {
            input.eat_ch();
            let directive = input.parse_ident()?;
            input.eat_spaces();

            //println!("{}", directive);

            // If defined
            if directive == "ifdef" {
                output += &process_ifdef(input, defs, counter, gen_output)?;
                continue
            }

            // If not defined
            if directive == "ifndef" {
                output += &process_ifndef(input, defs, counter, gen_output)?;
                continue
            }

            // On #else or #endif, stop
            if directive == "else" || directive == "endif" {
                return Ok((output, directive));
            }

            if gen_output && directive == "include" {
                let file_path = if input.peek_ch() == '<' {
                    let file_name = input.parse_str('>')?;
                    Path::new("include").join(file_name).display().to_string()
                }
                else
                {
                    // Compute the include path based on the
                    // current file's directory
                    let rel_include_path = input.parse_str('"')?;
                    let mut src_path = Path::new(&input.src_name).parent().unwrap();
                    src_path.join(rel_include_path).display().to_string()
                };

                let mut include_input = Input::from_file(&file_path)?;
                let (include_output, end_keyword) = process_input_rec(
                    &mut include_input,
                    defs,
                    counter,
                    gen_output
                )?;

                if end_keyword != "" {
                    return include_input.parse_error(&format!("unexpected #{}", end_keyword));
                }

                output += &include_output;

                // Emit # linenum filename directive
                // since we are returning to the parent file
                output += &format!("# {} \"{}\"\n", input.line_no, input.src_name);

                continue;
            }

            // Definition or macro
            if gen_output && directive == "define" {
                let def = parse_def(input)?;
                defs.insert(def.name.clone(), def);
                continue
            }

            // Undefine a macro or constant
            if gen_output && directive == "undef" {
                let name = input.parse_ident()?;
                defs.remove(&name);
                continue
            }

            if gen_output {
                return input.parse_error(&format!(
                    "unknown preprocessor directive {}", directive
                ));
            }
        }

        // If this is a character string or character literal
        if ch == '"' || ch == '\'' {
            output += &input.read_string(ch)?;
            continue;
        }

        // Eat single-line comments
        if input.match_chars(&['/', '/']) {
            // Copy the comment over to the output to preserve the source position
            let comment_str = input.collect(|input| Ok(input.eat_comment()))?;
            output += "//";
            output += &comment_str;
            continue;
        }

        // Eat multi-line comment
        if input.match_chars(&['/', '*']) {
            // Copy the comment over to the output to preserve the source position
            let comment_str = input.collect(|input| input.eat_multi_comment())?;
            output += "/*";
            output += &comment_str;
            continue;
        }

        // If this is an identifier
        if gen_output && is_ident_start(ch) {
            let ident = input.parse_ident()?;

            // If we have a definition for this identifier
            if let Some(def) = defs.get(&ident) {
                let def = def.clone();
                output += &expand_macro(input, defs, counter, gen_output, &def)?;
            }
            else if ident == "__LINE__" {
                output += &format!("{}", input.line_no);
            }
            else if ident == "__FILE__" {
                let mut filename: String = format!("\"{}\"", input.src_name);
                if cfg!(windows) {
                    filename = str::replace(&filename, "\\", "/");
                }
                output += &filename;
            }
            else if ident == "__COUNTER__" {
                output += &format!("{}", counter);
                *counter += 1;
            }
            else
            {
                output += &ident;
            }

            continue;
        }

        output.push(input.eat_ch());
    }

    Ok((output, "".to_string()))
}

#[cfg(test)]
mod tests
{
    use super::*;

    fn process(src: &str) -> String
    {
        let mut input = Input::new(&src, "src");
        process_input(&mut input).unwrap()
    }

    fn fails(src: &str)
    {
        let mut input = Input::new(&src, "src");
        assert!(process_input(&mut input).is_err());
    }

    fn line_count(src: &str) -> usize
    {
        let mut input = Input::new(&src, "src");
        process_input(&mut input).unwrap();
        input.line_no as usize
    }

    fn compile(file_name: &str) -> Result<(), ParseError>
    {
        use crate::parsing::Input;
        use crate::parser::parse_unit;
        use crate::cpp::process_input;

        dbg!(file_name);
        let mut input = Input::from_file(file_name)?;
        let output = process_input(&mut input)?;

        let mut input = Input::new(&output, file_name);
        let mut unit = parse_unit(&mut input)?;

        Ok(())
    }

    fn error_line(file_name: & str) -> usize
    {
        match compile(file_name) {
            Ok(_) => panic!(),
            Err(error) => error.line_no as usize
        }
    }

    #[test]
    fn empty()
    {
        assert_eq!(process(""), "");
        assert_eq!(process(" "), " ");
        assert_eq!(process("\n"), "\n");
    }

    #[test]
    fn macros()
    {
        fails("#define EOF(-1)");
    }

    #[test]
    fn lines()
    {
        assert_eq!(line_count("#define FOO 2\n"), 2);
        assert_eq!(line_count("#define FOO 2\nFOO"), 2);
        assert_eq!(line_count("#define FOO 2\nFOO\n"), 3);
    }

    #[test]
    fn error_lines()
    {
        assert_eq!(error_line("tests/line_nums/err_line_1.c"), 1);
        assert_eq!(error_line("tests/line_nums/err_line_2.c"), 2);
        assert_eq!(error_line("tests/line_nums/err_after_include.c"), 6);
        assert_eq!(error_line("tests/line_nums/err_after_include2.c"), 7);

        // Test error line numbers inside of include files
        assert_eq!(error_line("tests/line_nums/err_include_ln3.c"), 3);
    }
}
