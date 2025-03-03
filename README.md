# SMALLSH - A Simple Shell Implementation

## Introduction
This project is a simple shell implementation in C called `smallsh`. It replicates a subset of features found in popular shells like Bash. The shell provides a command-line interface for executing commands, managing processes, handling input/output redirection, and signal management.

## Features
- Provides a command prompt (`:`) for user input
- Supports blank lines and comments (`#` at the beginning of a line)
- Implements built-in commands:
- `exit`: Terminates the shell and kills any remaining child processes
- `cd`: Changes the current working directory
- `status`: Displays the exit status or termination signal of the last foreground command
- Executes other commands via the `exec()` family of functions
- Supports input (`<`) and output (`>`) redirection
- Handles both foreground and background processes (`&` for background execution)
- Implements custom handlers for signals:
- `SIGINT` (Ctrl+C): Terminates foreground processes but does not exit the shell
- `SIGTSTP` (Ctrl+Z): Toggles between foreground-only mode and normal mode

## Usage
Run the shell with:
```sh
./smallsh
```
Once running, you can enter commands. The syntax for commands is:
```sh
command [arg1 arg2 ...] [< input_file] [> output_file] [&]
```
- Commands are executed similarly to a Unix shell.
- `exit`, `cd`, and `status` are handled internally.
- Other commands invoke external programs using `execvp()`.
- Input and output redirection use `dup2()`.
- Background processes (`&`) return control to the shell immediately.
- Foreground processes run until completion.

## Built-in Commands
### `exit`
- Terminates the shell and kills any child processes before exiting.

### `cd [directory]`
- Without arguments, changes to the `HOME` directory.
- With an argument, changes to the specified directory.

### `status`
- Displays the exit status or termination signal of the last foreground process.

## Background and Foreground Processes
- Foreground commands run normally.
- Background commands (ending with `&`) run asynchronously, and their PIDs are displayed.
- When a background process completes, a message with its exit status is shown.

## Signal Handling
- `SIGINT` (Ctrl+C):
- The shell itself ignores `SIGINT`.
- Foreground processes terminate upon receiving `SIGINT`.
- Background processes ignore `SIGINT`.
- `SIGTSTP` (Ctrl+Z):
- Toggles foreground-only mode.
- In foreground-only mode, `&` is ignored, and all processes run in the foreground.
- Sending `SIGTSTP` again restores normal behavior.

## Example Usage
```sh
$ ./smallsh
: ls
file1.txt file2.txt smallsh.c
: cd /home/user
: pwd
/home/user
: echo "Hello, world!" > output.txt
: cat < output.txt
Hello, world!
: sleep 10 &
background pid is 12345
: kill 12345
background pid 12345 is done: terminated by signal 15
: ^Z
Entering foreground-only mode (& is now ignored)
: sleep 5 &
: date
Mon Jan 2 11:24:38 PST 2025
: ^Z
Exiting foreground-only mode
: exit
```

## Implementation Details
- Uses `fork()` to create child processes.
- Uses `execvp()` to run external commands.
- Uses `waitpid()` to manage child processes.
- Uses `dup2()` for I/O redirection.
- Uses signal handlers for `SIGINT` and `SIGTSTP`.

## Compilation
Compile the program with:
```sh
gcc -o smallsh smallsh.c
```

## Notes
- The shell does not support command piping (`|`).
- No syntax validation is performed on user input.



