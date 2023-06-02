#
# Game where you try to guess the computer's number
# in as few guesses as you can.
# You are told whether your guess is too high or too low.
#

.data;

INTRO_STR: .stringz "I'm thinking of a number between 0 and 1000.\n";
PROMPT_STR: .stringz "Guess: ";
LOW_STR: .stringz "Too low. Try again.\n";
HIGH_STR: .stringz "Too high. Try again.\n";
WIN_STR0: .stringz "You got it! The number is ";
WIN_STR1: .stringz ". Number of guesses: ";

.code;

# num_guesses, local 0
push 0;

# Calculate the target number to be guessed.
syscall time_current_ms;
push 999;
mod_u64;
push 1;
add_u64;

# target, local 1, is now some value in range [1, 999]

push INTRO_STR;
syscall print_str;

GUESS:
# num_guesses += 1
get_local 0;
push 1;
add_u64;
set_local 0;

# Ask user for their guess.
push PROMPT_STR;
syscall print_str;
syscall read_i64;

# if (guess == target) goto WIN;
dup;
get_local 1;
eq_u64;
jnz WIN;

# else if (guess < target) goto TOO_LOW;
get_local 1;
lt_i64;
jnz TOO_LOW;

# else guess > target
push HIGH_STR;
syscall print_str;
jmp GUESS;

TOO_LOW:
push LOW_STR;
syscall print_str;
jmp GUESS;

WIN:
# Tell the user they won
# and how many guesses they made.

push WIN_STR0;
syscall print_str;

# Print the target number.
pop; # Pop off the correct guess.
# target, local 1, is at top of stack.
syscall print_i64;

push WIN_STR1;
syscall print_str;

# Print the number of guesses made.
# num_guesses, local 0, is the last value on the stack.
syscall print_i64;
syscall print_endl;

push 0;
exit;
