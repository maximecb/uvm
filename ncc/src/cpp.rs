// Reference:
// https://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html

use std::path::Path;
use std::collections::HashMap;
use crate::parsing::*;

struct Macro
{
    name: String,
    params: Vec<String>,
    text: String,
}

fn parse_macro(input: &mut Input) -> Result<Macro, ParseError>
{
    let name = input.parse_ident()?;
    let mut params = Vec::default();

    if input.match_token("(")? {
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

            text.push('\n');
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

/// Ignore contents until #else or #endif
/// This returns the end keyword that was found
fn ignore_contents(input: &mut Input) -> Result<String, ParseError>
{




    todo!();

}

fn process_ifndef(input: &mut Input, defs: &mut HashMap<String, Macro>) -> Result<String, ParseError>
{
    let ident = input.parse_ident()?;
    let is_defined = defs.get(&ident).is_some();

    let mut output = String::new();

    // If not defined
    if !is_defined {
        // Process the then branch normally
        let mut end_keyword = None;
        output += &process_input_rec(input, true, &mut end_keyword)?;

        // If there is an else branch
        if end_keyword.unwrap() == "else" {
            // Ignore the output until the end
            let end_keyword = ignore_contents(input)?;

            if end_keyword != "endif" {
                return input.parse_error("expected #endif");
            }
        }
    }
    else
    {
        // Name defined, we need to ignore the then branch








    }

    Ok(output)
}

/// Process the input and generate an output string
pub fn process_input(input: &mut Input) -> Result<String, ParseError>
{
    process_input_rec(input, false, &mut None)
}

/// Process the input and generate an output string recursively
pub fn process_input_rec(
    input: &mut Input,
    inside_branch: bool,
    end_keyword: &mut Option<String>
) -> Result<String, ParseError>
{
    let mut output = String::new();
    let mut defs = HashMap::new();

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
            input.eat_ws()?;

            //println!("{}", directive);

            if directive == "include" {
                let file_path = if input.peek_ch() == '<' {
                    let file_name = input.parse_str('>')?;
                    Path::new("include").join(file_name).display().to_string()
                }
                else
                {
                    input.parse_str('"')?
                };

                let mut input = Input::from_file(&file_path);
                let include_output = process_input(&mut input)?;

                // TODO: emit linenum directive

                output += &include_output;

                // TODO: emit linenum directive

                continue;
            }

            // Definition or macro
            if directive == "define" {
                let def = parse_macro(input)?;
                defs.insert(def.name.clone(), def);
                continue
            }

            // Undefine a macro or constant
            if directive == "undef" {
                let name = input.parse_ident()?;
                defs.remove(&name);
                continue
            }

            // If not defined
            if directive == "ifndef" {
                output += &process_ifndef(input, &mut defs)?;
                continue
            }

            // On #else or #endif, stop
            if inside_branch {
                if directive == "#else" {
                    *end_keyword = Some(directive);
                    break;
                }

                if directive == "#endif" {
                    *end_keyword = Some(directive);
                    break;
                }
            }

            return input.parse_error(&format!(
                "unknown preprocessor directive {}", directive
            ));
        }

        // TODO: eat comments
        // We don't want to preprocess things inside comments

        // TODO: keep track if we're inside of a string or not
        // We don't want to preprocess things inside strings

        // TODO: we need to parse defines
        // Can naively match against all identifiers
        // Note that we only need to care about ident chars
        // we could read the char and then match instead

        // If this is an identifier
        if is_ident_ch(ch) {
            let ident = input.parse_ident()?;

            // If we have a definition for this identifier
            if let Some(def) = defs.get(&ident) {






            }

            output += &ident;
            continue;
        }

        output.push(input.eat_ch());
    }

    Ok(output)
}
