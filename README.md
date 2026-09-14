# Custom C Shell (`myshell`)

A lightweight Unix command-line shell implemented in C. This custom shell supports executing single commands, handling environment variable expansions, pipeline operations, output redirections, parallel and sequential command executions, quote parsing, and custom signal handling.

---

## Features

- **Built-in Commands:**
  - `cd <path>`: Changes the current working directory.
  - `exit`: Cleanly terminates the shell prompt.
- **Environment & Variable Expansion:**
  - Expands `$VAR` to environment variable values using `getenv`.
  - Expands `$$` to the process ID (PID) of the current running shell.
- **Execution Modes:**
  - **Single Execution:** Executes standard Unix commands (e.g., `ls -la`).
  - **Sequential Execution (`##`):** Runs commands one after another in order (`cmd1 ## cmd2`).
  - **Parallel Execution (`&&`):** Forks and executes commands concurrently, waiting for all child processes to complete (`cmd1 && cmd2`).
- **I/O Redirection & Pipelines:**
  - **Output Redirection (`>`):** Redirects stdout to a specified target file (`cmd > file.txt`).
  - **Pipeline Support (`|`):** Connects stdout of one process to stdin of the next across multiple commands (`cmd1 | cmd2 | cmd3`).
- **Quote-Aware Tokenizer:**
  - Correctly handles spaces and special syntax symbols inside single (`'`) or double (`"`) quotes.
- **Signal Handling:**
  - Ignores `SIGINT` (`Ctrl+C`) and `SIGTSTP` (`Ctrl+Z`) in the parent interactive shell. Child processes reset these back to default behaviors (`SIG_DFL`).

---

## Syntax & Operators

| Operator | Function | Example |
| :--- | :--- | :--- |
| `##` | Sequential Execution | `echo First ## echo Second` |
| `&&` | Parallel Execution | `sleep 2 && sleep 2` |
| `>` | File Output Redirection | `ls -l > dir_list.txt` |
| `\|` | Inter-Process Piping | `cat myshell.c \| grep main` |

---

## Project Structure

```text
.
├── myshell.c    # Complete C implementation (Parser, Process Management, Signals)
└── README.md    # Project documentation
```

---

## Compilation & Usage

### 1. Build
Compile the code using `gcc`:

```bash
gcc -Wall -Wextra myshell.c -o myshell
```

### 2. Execution
Launch the interactive shell:

```bash
./myshell
```

### 3. Example Usage

- **Directory Navigation:**
  ```bash
  /home/user$ cd /tmp
  ```

- **Variable & Process ID Expansion:**
  ```bash
  /home/user$ echo "Logged in user: $USER (Shell PID: $$)"
  ```

- **Parallel Command Execution (`&&`):**
  ```bash
  /home/user$ sleep 2 && echo "Task 1 completed" && echo "Task 2 completed"
  ```

- **Sequential Command Execution (`##`):**
  ```bash
  /home/user$ mkdir test_folder ## cd test_folder ## pwd
  ```

- **Piping & Redirection Combo:**
  ```bash
  /home/user$ cat myshell.c | grep "void" > function_declarations.txt
  ```

---

## Technical Details & Design

1. **Memory & Parsing:** Memory allocation and string tokens are handled dynamically. Quoted strings strip outer quotes after tokenization to pass raw string literals to `execvp()`.
2. **Zombie Process Prevention:** When executing commands in parallel (`&&`), child PIDs are tracked in an array, and the parent process calls `waitpid()` for each PID before returning to the prompt.
3. **Operator Precedence:** The shell parses syntax elements in the order: `&&` $
ightarrow$ `##` $
ightarrow$ `|` $
ightarrow$ `>`. Operators nested within quotes are safely ignored during parsing.
