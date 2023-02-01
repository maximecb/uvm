// Reference:
// https://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html

use crate::parsing::*;

/// Process the input and generate an otput string
fn process_input(input: &mut Input) -> Result<String, ParseError>
{
    let mut output: String = String::new();

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

            if directive == "include" {
                input.eat_ws()?;
                let file_name = input.parse_str()?;

                // TODO: need to support square brackets #include <file_name>

                let mut input = Input::from_file(&file_name);
                let include_output = process_input(&mut input)?;

                // TODO: emit linenum directive

                output += &include_output;

                // TODO: emit linenum directive

                continue;
            }

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









        output.push(input.eat_ch());
    }

    Ok(output)
}
