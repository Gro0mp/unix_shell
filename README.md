# bsh - A Basic Shell

## Overview
`bsh` is a simple Unix shell that supports basic shell functionalities, including executing commands, managing environment variables, changing directories, handling input/output redirection, and maintaining a command history.

## Features
- Supports execution of system commands via `execv`
- Built-in commands:
  - `exit` - Exit the shell
  - `env` - Display all environment variables
  - `setenv VAR VALUE` - Set an environment variable
  - `unsetenv VAR` - Unset an environment variable
  - `cd [dir]` - Change directory (defaults to `$HOME` if no directory is specified)
  - `history` - Show command history
- Maintains a history of the last 500 commands
- Supports input (`<`) and output (`>`) redirection
- Searches executable paths using `$PATH`

## Compilation
To compile the shell, run:
```sh
make
```
This will generate an executable named `bsh`.

## Usage
Run the shell by executing:
```sh
./bsh
```
Once inside the shell, enter commands as you would in a typical Unix shell.

## Example Commands
```sh
bsh> ls -l
bsh> setenv MYVAR hello
bsh> env
bsh> cd /usr/local
bsh> history
bsh> cat < input.txt > output.txt
```

## Cleanup
To remove the compiled executable, use:
```sh
make clean
```

## Notes
- The shell does not support background process execution (`&`).
- Only supports basic I/O redirection (single `<` and `>`).
- No support for pipes (`|`) or job control.
