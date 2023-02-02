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

/// Process the input and generate an otput string
pub fn process_input(input: &mut Input) -> Result<String, ParseError>
{
    let mut output: String = String::new();

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



            /*
            if directive == "ifndef" {

                continue
            }
            */

            return input.parse_error(&format!(
                "unknown preprocessor directive {}", directive
            ));
        }

        // TODO: eat comments

        // TODO: keep track if we're inside of a string or not

        // Preprocessor needs to be aware if it's inside of a string or inside a comment

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
