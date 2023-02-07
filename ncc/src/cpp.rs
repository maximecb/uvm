// Reference:
// https://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html

use std::path::Path;
use std::collections::HashMap;
use crate::parsing::*;

impl Input
{
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
}

#[derive(Clone, Debug)]
struct Macro
{
    name: String,
    params: Vec<String>,
    text: String,
}

fn parse_macro(input: &mut Input) -> Result<Macro, ParseError>
{
    let name = input.parse_ident()?;
    input.eat_spaces();

    let mut params = Vec::default();

    if input.match_chars(&['(']) {
        loop
        {
            if input.match_token(")")? {
                break;
            }

            if input.eof() {
                return input.parse_error("eof inside define directive");
            }

            params.push(input.parse_ident()?);

            if input.match_token(")")? {
                break;
            }

            input.expect_token(",")?;
        }
    }

    // Read text until \n
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

    Ok(Macro {
        name,
        params,
        text,
    })
}

fn process_ifndef(
    input: &mut Input,
    defs: &mut HashMap<String, Macro>,
    gen_output: bool,
) -> Result<String, ParseError>
{
    let ident = input.parse_ident()?;
    let is_defined = defs.get(&ident).is_some();

    let mut output = String::new();

    // If not defined
    if !is_defined {
        // Process the then branch normally
        let mut end_keyword = "".to_string();
        output += &process_input_rec(
            input,
            defs,
            gen_output,
            &mut end_keyword
        )?;

        // If there is an else branch
        if end_keyword == "else" {
            let mut end_keyword = "".to_string();
            process_input_rec(
                input,
                defs,
                false,
                &mut end_keyword
            )?;

            if end_keyword != "endif" {
                return input.parse_error("expected #endif");
            }
        }
    }
    else
    {
        // Name defined, we need to ignore the then branch
        let mut end_keyword = "".to_string();
        process_input_rec(
            input,
            defs,
            false,
            &mut end_keyword
        )?;

        // If there is an else branch
        if end_keyword == "else" {
            let mut end_keyword = "".to_string();
            output += &process_input_rec(
                input,
                defs,
                gen_output,
                &mut end_keyword
            )?;

            if end_keyword != "endif" {
                return input.parse_error("expected #endif");
            }
        }
    }

    Ok(output)
}

/// Expand a definition or macro
fn expand_macro(
    input: &mut Input,
    defs: &mut HashMap<String, Macro>,
    gen_output: bool,
    def: &Macro,
) -> Result<String, ParseError>
{
    let mut args = Vec::new();

    // Parse the macro arguments
    if def.params.len() > 0 {
        input.expect_token("(")?;

        loop
        {
            if input.eof() {
                return input.parse_error("unexpected end of input");
            }

            if input.match_token(")")? {
                break;
            }

            // Read the argument
            let mut arg = "".to_string();
            let mut paren_level: usize = 0;
            loop
            {
                if input.eof() {
                    break;
                }

                let ch = input.peek_ch();

                match ch {
                    '(' => {
                        paren_level += 1
                    }
                    ')' => {
                        if paren_level == 0 { break; }
                        paren_level -= 1;
                    }
                    ',' => {
                        if paren_level == 0 { break; }
                    }
                    _ => {}
                }

                arg.push(input.eat_ch());
            }
            args.push(arg);

            if input.match_token(")")? {
                break;
            }

            input.expect_token(",")?;
        }
    }

    if args.len() != def.params.len() {
        return input.parse_error(&format!(
            "macro {} expected {} arguments",
            def.name,
            def.params.len()
        ));
    }

    let mut text = def.text.clone();

    // Replace the parameters by their value
    for (idx, param) in def.params.iter().enumerate() {
        text = text.replace(param, &args[idx]);
    }

    Ok(text)
}

/// Process the input and generate an output string
pub fn process_input(input: &mut Input) -> Result<String, ParseError>
{
    let mut defs = HashMap::new();

    let mut end_keyword = "".to_string();
    let result = process_input_rec(
        input,
        &mut defs,
        true,
        &mut end_keyword
    );

    if end_keyword != "" {
        return input.parse_error(&format!("unexpected #{}", end_keyword));
    }

    result
}

/// Process the input and generate an output string recursively
fn process_input_rec(
    input: &mut Input,
    defs: &mut HashMap<String, Macro>,
    gen_output: bool,
    end_keyword: &mut String
) -> Result<String, ParseError>
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

            // If not defined
            if directive == "ifndef" {
                output += &process_ifndef(
                    input,
                    defs,
                    gen_output
                )?;
                continue
            }

            // On #else or #endif, stop
            if directive == "else" || directive == "endif" {
                *end_keyword = directive;
                break;
            }

            if gen_output && directive == "include" {
                let file_path = if input.peek_ch() == '<' {
                    let file_name = input.parse_str('>')?;
                    Path::new("include").join(file_name).display().to_string()
                }
                else
                {
                    input.parse_str('"')?
                };

                let mut input = Input::from_file(&file_path);

                let mut end_keyword = "".to_string();
                let include_output = process_input_rec(
                    &mut input,
                    defs,
                    gen_output,
                    &mut end_keyword
                )?;

                if end_keyword != "" {
                    return input.parse_error(&format!("unexpected #{}", end_keyword));
                }

                // TODO: emit linenum directive

                output += &include_output;

                // TODO: emit linenum directive

                continue;
            }

            // Definition or macro
            if gen_output && directive == "define" {
                let def = parse_macro(input)?;
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

        // Eat single-line comments
        if input.match_chars(&['/', '/']) {
            input.eat_comment();
            // Do we want to copy over the content to the output to
            // avoid messing up the source position?
            continue;
        }

        // Eat multi-line comment
        if input.match_chars(&['/', '*']) {
            input.eat_multi_comment()?;
            // Do we want to copy over the content to the output to
            // avoid messing up the source position?
            continue;
        }

        // Keep track if we're inside of a string or not
        // We don't want to preprocess things inside strings
        if input.match_chars(&['"']) {
            output.push('"');
            loop
            {
                if input.eof() {
                    return input.parse_error("unexpected end of input inside string");
                }

                let ch = input.eat_ch();
                output.push(ch);

                if ch == '"' {
                    break;
                }

                if ch == '\\' {
                    let ch = input.eat_ch();
                    output.push(ch);
                    continue;
                }
            }

            continue;
        }

        // If this is an identifier
        if gen_output && is_ident_ch(ch) {
            let ident = input.parse_ident()?;

            // If we have a definition for this identifier
            if let Some(def) = defs.get(&ident) {
                let def = def.clone();
                output += &expand_macro(input, defs, gen_output, &def)?;
            }
            else
            {
                output += &ident;
            }

            continue;
        }

        output.push(input.eat_ch());
    }

    Ok(output)
}
