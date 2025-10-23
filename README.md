# Novash
**A minimal Unix-like shell written in C**

A lightweight shell inspired by the [Build your own Shell](https://app.codecrafters.io/courses/shell/overview) roadmap from CodeCrafters.
The goal is to implement, from scratch the essential components of a modern UNIX shell including parsing, executing, background jogs and user interaction.

---

## Features

### Parsing

- [x] Tokenization (handles quotes and metacharacters)
- [x] Abstract Syntax Tree (AST) construction following a Bash-like grammar:
  ```bash
  cmd arg1 -arg2 --arg3  # command and arguments
  cmd arg1 < input.txt >> foo.txt 2> /dev/null  # redirections
  cmd1 | cmd2 | cmd3     # pipelines
  cmd1 && cmd2 || cmd3   # conditional execution
  cmd1 &; cmd2; cmd3     # sequential execution background tasks
  ```

### Execution
- [X] Builtins (`cd`, `echo`, `pwd`, `exit`, `type`, `history`)
- [X] Redirections and pipelines
- [X] Conditional and sequential execution (`&&`, `||`, `;`)
- [X] Background tasks (`&`)
- [X] Signal handling (`SIGINT`, `SIGCHLD`)
- [ ] Job management (`fg`, `bg`, `jobs`)

## Command Line navigation

- [ ] Integrate GNU Readline or linenoise for input handling, history search and completion

### History

- [X] Persistent across sessions
- [X] Stored in a file and loaded at startup
- [ ] History expansion (`!cmd`)

### Command Line

- [X] Line editing (GNU Readline or linenoise)
- [ ] Auto-completion 

--- 

## Build and Run

### Clone the repository
```sh
git clone https://github.com/thomas-gons/Novash.git
cd Novash
```

### Make the utility script executable and run it
```sh
# Make the utility script executable and run it
chmod u+x run.sh
./run.sh
```
Optional flags:
   -d, --debug   Run in debug mode
   -h, --help    Show the help message
