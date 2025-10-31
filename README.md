# Novash
**A minimal Unix-like shell written in C**

A lightweight shell inspired by the [Build your own Shell](https://app.codecrafters.io/courses/shell/overview) roadmap from CodeCrafters.
The goal is to implement, from scratch the essential components of a modern UNIX shell including parsing, executing, background jobs and user interaction.

---

## Architecture

Novash is built with a modular architecture consisting of:

- **Lexer** (`src/lexer/`) - Tokenization with quote handling and metacharacter recognition
- **Parser** (`src/parser/`) - AST construction following a Bash-like grammar
- **Executor** (`src/executor/`) - Command execution with process and job management
- **Builtins** (`src/builtin/`) - Internal shell commands
- **History** (`src/history/`) - Persistent command history with circular buffer
- **Shell** (`src/shell/`) - Main REPL loop, state management, and signal handling
- **Utils** (`src/utils/`) - Memory management, system call wrappers, and utility functions

---

## Features

### Lexer & Tokenization

- [x] Tokenization with support for:
  - Words and commands
  - Quotes (single `'` and double `"`) with proper nesting
  - Metacharacters (`|`, `&`, `;`, `<`, `>`, `>>`, `&&`, `||`)
  - File descriptor tokens for redirections
- [x] Quote removal in token values while tracking raw input length
- [x] Whitespace and metacharacter boundary detection

### Parser & AST Construction

- [x] Abstract Syntax Tree (AST) with four node types:
  - **Command nodes** - simple commands with arguments and redirections
  - **Pipeline nodes** - commands connected by `|`
  - **Conditional nodes** - commands with `&&` or `||` operators
  - **Sequence nodes** - commands separated by `;` or `&`
- [x] Recursive descent parser following shell grammar:
  ```bash
  cmd arg1 -arg2 --arg3  # command and arguments
  cmd arg1 < input.txt >> foo.txt 2> /dev/null  # redirections
  cmd1 | cmd2 | cmd3     # pipelines
  cmd1 && cmd2 || cmd3   # conditional execution
  cmd1 &; cmd2; cmd3     # sequential execution and background tasks
  ```
- [x] Redirection parsing with file descriptor support (`0`, `1`, `2`)
- [x] Background task detection (`&`)
- [x] Raw command string preservation for display

### Execution Engine

- [x] Process management with fork/exec model
- [x] Process group management for job control
- [x] Synchronization using pipes for race-free process group setup
- [x] Signal masking during critical sections
- [x] Pipeline execution with proper pipe setup
- [x] Conditional execution (`&&` returns on failure, `||` returns on success)
- [x] Sequential execution with `;` separator
- [x] Background task execution (`&`)
- [x] I/O redirections:
  - Input redirection (`<`)
  - Output redirection (`>`)
  - Append redirection (`>>`)
  - File descriptor redirection (e.g., `2>`)

### Builtin Commands

- [x] **`cd`** - Change directory (supports `~` for home)
- [x] **`echo`** - Print arguments to stdout
- [x] **`pwd`** - Print current working directory
- [x] **`exit`** - Exit the shell (with running job warning)
- [x] **`type`** - Display command type (builtin or external with path)
- [x] **`history`** - Display command history
- [x] **`jobs`** - List background jobs with status
- [x] **`fg`** - Bring background job to foreground
- [x] **`bg`** - Resume stopped job in background

### Job Control

- [x] Job data structures with process pipeline support
- [x] Job states: `RUNNING`, `DONE`, `STOPPED`, `KILLED`
- [x] Process states: `RUNNING`, `DONE`, `STOPPED`, `KILLED`
- [x] Job list management (add, remove, find by PGID)
- [x] Process tracking within jobs
- [x] Background job completion notification
- [x] Foreground/background job control (`fg`, `bg`, `jobs`)
- [x] Job status display with command line

### Signal Handling

- [x] **SIGINT** - Graceful handling of Ctrl+C
  - Cleans up readline interface
  - Redisplays prompt without terminating shell
- [x] **SIGCHLD** - Asynchronous child process reaping
  - Non-blocking waitpid with `WNOHANG`
  - Handles exited, killed, stopped, and continued processes
  - Updates job and process states
  - Notifies completion of background jobs
- [x] **SIGTTOU/SIGTTIN** - Ignored for background job control
- [x] Event-driven signal processing via readline hooks

### Terminal Control

- [x] Shell process group management
- [x] Terminal ownership with `tcsetpgrp`
- [x] Terminal mode preservation and restoration
- [x] Foreground process group control
- [x] TTY detection with fallback for non-interactive mode

### History Management

- [x] Persistent command history across sessions
- [x] Circular buffer implementation (configurable size: 1000 entries)
- [x] Stored in `.nsh_history` file
- [x] Automatic loading at shell startup
- [x] Immediate save after each command
- [x] History trimming to enforce size limits
- [x] Integration with GNU Readline history
- [x] Timestamp tracking for commands

### Shell State

- [x] Global singleton state management
- [x] Environment variable hashmap (`HOME`, `PATH`, `SHELL`, `HISTFILE`)
- [x] Current working directory tracking
- [x] Last exit status tracking
- [x] Last foreground command tracking
- [x] Job list with running job counter
- [x] Terminal modes and process group ID

### Command Line Interface

- [x] GNU Readline integration for line editing
- [x] Command history navigation (up/down arrows)
- [x] Line editing capabilities (emacs/vi modes)
- [x] Customizable prompt (`$ `)
- [x] EOF (Ctrl+D) handling with running job warning
- [x] Interrupt handling (Ctrl+C) without shell exit
- [ ] Auto-completion
- [ ] History expansion (`!cmd`)

### Memory Management & Utilities

- [x] Safe memory allocation wrappers (`xmalloc`, `xcalloc`, `xrealloc`, `xstrdup`)
- [x] OOM (Out Of Memory) handling with error exit
- [x] System call wrappers with error handling (`xfork`, `xpipe`, `xdup2`, `xopen`, `xsetpgid`)
- [x] Dynamic arrays using `stb_ds` (stretchy buffers and hashmaps)
- [x] PATH environment variable search for executables
- [x] Configurable logging system (NONE, ERR, WARN, INFO, DEBUG levels)
- [x] Color-coded log output with file/line information 

---

## Build and Run

### Prerequisites

- **C Compiler**: GCC or Clang with C23 support
- **CMake**: Version 3.16 or higher
- **GNU Readline**: Development library and headers
- **Criterion**: Testing framework (for tests)

Install dependencies on Ubuntu/Debian:
```sh
sudo apt-get install build-essential cmake libreadline-dev libcriterion-dev
```

### Clone the repository
```sh
git clone https://github.com/thomas-gons/Novash.git
cd Novash
```

### Quick Start with the utility script
```sh
# Make the utility script executable and run it
chmod u+x run.sh
./run.sh
```
Optional flags:
```
  -d, --debug       Build in Debug mode
  -b, --build-only  Only build, do not run shell or tests
  -t, --test        Build and run tests
  -c, --clean       Remove the build directories and exit
  -h, --help        Display this help message
```

### Or build manually with CMake

#### 1. Create build directory
```sh
mkdir build
cd build
```

#### 2. Configuration options
```sh
# Release build (optimized, -O3)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Debug build with symbols and no optimization
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Debug build with sanitizers (AddressSanitizer, UndefinedBehaviorSanitizer)
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON

# Set log level (NONE, ERR, WARN, INFO, DEBUG)
cmake .. -DLOG_LEVEL=INFO

# Treat warnings as errors
cmake .. -DWARNINGS_AS_ERRORS=ON
```

#### 3. Build the project
```sh
# Build the shell
cmake --build . -j$(nproc) --target nsh

# Build tests
cmake --build . -j$(nproc) --target tests_novash

# Build everything
cmake --build . -j$(nproc)
```

#### 4. Run the shell
```sh
# Run the shell (from build directory)
./nsh

# Or run from project root
./build/nsh
```

#### 5. Run tests
```sh
# Run tests (from build directory)
./tests_novash

# Or using CTest
ctest --output-on-failure
```

### Build Targets

- **`nsh`** - Main shell executable
- **`novash_core`** - Core library with all shell functionality
- **`tests_novash`** - Test suite
- **`format`** - Format source files with clang-format

---

## Project Structure

```
Novash/
├── src/
│   ├── main.c              # Entry point
│   ├── builtin/            # Built-in commands
│   │   ├── builtin.c       # Builtin registry and core builtins
│   │   ├── builtin.h
│   │   ├── history.c       # History builtin command
│   │   └── job_control.c   # Job control builtins (fg, bg, jobs)
│   ├── executor/           # Command execution engine
│   │   ├── executor.c      # AST execution and process management
│   │   ├── executor.h
│   │   ├── jobs.c          # Job control data structures
│   │   └── jobs.h
│   ├── history/            # History management
│   │   ├── history.c       # Persistent command history
│   │   └── history.h
│   ├── lexer/              # Lexical analysis
│   │   ├── lexer.c         # Tokenization with quote handling
│   │   └── lexer.h
│   ├── parser/             # Syntax analysis
│   │   ├── parser.c        # AST construction
│   │   └── parser.h
│   ├── shell/              # Shell core
│   │   ├── config.h        # Configuration constants
│   │   ├── shell.c         # Main REPL loop
│   │   ├── shell.h
│   │   ├── signal.c        # Signal handling
│   │   ├── signal.h
│   │   ├── state.c         # Global state management
│   │   └── state.h
│   └── utils/              # Utility functions
│       ├── collections.c   # Dynamic arrays and hashmaps (stb_ds)
│       ├── collections.h
│       ├── log.h           # Logging macros
│       ├── utils.c         # PATH search and utilities
│       ├── utils.h
│       └── system/         # System call wrappers
│           ├── memory.c    # Safe memory allocation
│           ├── memory.h
│           ├── syscall.c   # System call wrappers
│           └── syscall.h
├── tests/                  # Test suite
│   ├── test_lexer.c
│   └── test_parser.c
├── external/               # External dependencies (stb_ds.h)
├── CMakeLists.txt          # Build configuration
├── run.sh                  # Build and run utility script
├── Doxyfile                # Doxygen documentation config
└── README.md
```

---

## Usage Examples

### Basic Commands
```sh
$ echo Hello, Novash!
Hello, Novash! 

$ pwd
/home/user/Novash

$ cd /tmp
$ pwd
/tmp

$ type echo
echo is a shell builtin

$ type ls
ls is /usr/bin/ls
```

### Redirections
```sh
$ echo "Hello" > output.txt
$ cat < output.txt
Hello

$ ls -la >> files.txt
$ cat files.txt 2> /dev/null
```

### Pipelines
```sh
$ ls -l | grep ".c" | wc -l
$ cat file.txt | sort | uniq
```

### Conditional Execution
```sh
$ mkdir test && cd test && pwd
/home/user/test

$ false || echo "This will run"
This will run

$ true && echo "Success" || echo "Failure"
Success
```

### Background Jobs
```sh
$ sleep 10 &
[1] 12345

$ jobs
[1]+ Running                 sleep 10 &

$ fg %1
sleep 10
^Z
[1]+ Stopped                 sleep 10

$ bg %1
[1]+ Running                 sleep 10 &
```

### History
```sh
$ history
1  echo Hello
2  pwd
3  cd /tmp
4  ls -la
5  history
```

---

## Configuration

### Environment Variables

Novash automatically sets and uses the following environment variables:

- **`HOME`** - User's home directory
- **`PATH`** - Executable search path
- **`SHELL`** - Path to the Novash executable
- **`HISTFILE`** - Path to command history file (`.nsh_history`)

### Configuration Constants

Defined in `src/shell/config.h`:

- **`HIST_SIZE`** - Maximum history entries (default: 1000)
- **`HIST_FILENAME`** - History file name (default: `.nsh_history`)
- **`COMMAND_NOT_FOUND_EXIT_CODE`** - Exit code for missing commands (127)
- **`JOB_STOPPED_EXIT_CODE`** - Exit code for stopped jobs (146)

### Build-time Configuration

- **Log Level**: Set via `LOG_LEVEL` CMake variable (`NONE`, `ERR`, `WARN`, `INFO`, `DEBUG`)
- **Sanitizers**: Enable with `-DENABLE_SANITIZERS=ON` for memory debugging
- **Warnings as Errors**: Enable with `-DWARNINGS_AS_ERRORS=ON`

---

## Technical Details

### Grammar

Novash implements a shell grammar with operator precedence:

```
sequence     → conditional (( ';' | '&' ) conditional)*
conditional  → pipeline (( '&&' | '||' ) pipeline)*
pipeline     → command ( '|' command )*
command      → WORD* redirection*
redirection  → FD? ( '<' | '>' | '>>' ) WORD
```

### Process Execution Model

1. **Fork**: Create child process
2. **Process Group**: Set up process group for job control
3. **Synchronization**: Use pipe to avoid race conditions
4. **Redirections**: Set up file descriptors
5. **Exec**: Replace child process image
6. **Wait**: Parent waits for foreground jobs or monitors background jobs

### Signal Flow

- **SIGINT (Ctrl+C)**: Delivered to foreground process group, shell updates display
- **SIGCHLD**: Async notification of child state changes, handled in event loop
- **SIGTTOU/SIGTTIN**: Ignored to allow background job terminal operations

### Memory Safety

- All allocations use safe wrappers (`xmalloc`, etc.) with OOM handling
- Dynamic arrays automatically resize with `stb_ds`
- AST nodes and tokens properly freed after use
- History uses circular buffer to prevent unbounded growth

---

## Testing

Novash uses the Criterion testing framework:

```sh
# Run all tests
./run.sh -t

# Or manually
cmake --build build --target tests_novash
./build/tests_novash
```

Test coverage includes:
- Lexer tokenization with various quote combinations
- Parser AST construction for complex command structures
- Redirection parsing with file descriptors

---

## Contributing
Contributions are welcome! Feel free to open issues or submit pull requests on GitHub.

### License
This project is licensed under the GNU General Public License v3 or later. See the [LICENSE](LICENSE) file for details.
