# Minishell

## Overview

I built a Bash-like shell that provides a text-based user interface for interacting with the operating system. This project helped me:

- Implement process creation and management using `fork()` to create child processes
- Develop I/O redirection and piping between processes
- Handle command execution and proper error management

The resulting application is a functional command-line shell that supports many of the core features found in standard shells like Bash.

## Features Implemented

### Basic Navigation and Shell Controls
- **Directory Navigation**: Implemented the `cd` command for changing directories with support for both absolute and relative paths
- **Current Directory**: Added the `pwd` command to display the current working directory
- **Shell Exit**: Implemented `quit` and `exit` commands to properly terminate the shell

### Command Execution
- Created a system for running executables by spawning child processes
- Supported both absolute paths (e.g., `/usr/bin/ls`) and relative paths (e.g., `./program`)
- Implemented proper error handling for invalid commands

### Environment Variables
- Added support for environment variable usage and manipulation
- Implemented variable assignment (`NAME="John Doe"`)
- Supported variable substitution (`$NAME`)
- Handled undefined variables by evaluating them to empty strings

### Advanced Operators

#### Sequential Execution (`;`)
- Implemented sequential command execution using the `;` operator
- Ensured commands run one after another in the specified order

#### Parallel Execution (`&`)
- Added support for running commands in parallel using the `&` operator
- Managed multiple concurrent processes correctly

#### Piping (`|`)
- Implemented the pipe operator to connect the standard output of one command to the standard input of another
- Supported multiple pipes in a single command

#### Conditional Execution (`&&` and `||`)
- Added conditional execution with logical AND (`&&`) - continuing execution while commands succeed
- Implemented logical OR (`||`) - continuing execution while commands fail
- Adhered to proper operator precedence rules

### I/O Redirection
- **Input Redirection**: `< filename` redirects file content to standard input
- **Output Redirection**: `> filename` redirects standard output to a file
- **Error Redirection**: `2> filename` redirects standard error to a file
- **Combined Redirection**: `&> filename` redirects both standard output and error to a file
- **Append Mode**: Implemented `>>` and `2>>` for appending to files instead of overwriting

## Implementation Details

I implemented the shell by:
1. Creating a command parser to interpret user input
2. Using system calls like `fork()`, `execvp()`, and `waitpid()` to create and manage child processes
3. Setting up pipes between processes using the `pipe()` system call
4. Redirecting standard input/output/error using `dup2()`
5. Managing environment variables in a structured way
6. Implementing proper signal handling and process cleanup
