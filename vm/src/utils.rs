/// Produce a string with comma separator for thousands for an integer
pub fn thousands_sep(n: u64) -> String
{
    let num_chars: Vec<char> = n.to_string().chars().rev().collect();

    let mut chars_sep = Vec::new();

    for idx in 0..num_chars.len() {
        chars_sep.push(num_chars[idx]);
        if (idx % 3) == 2 && idx < num_chars.len() - 1 {
            chars_sep.push(',');
        }
    }

    let num_str: String = chars_sep.into_iter().rev().collect();

    num_str
}
