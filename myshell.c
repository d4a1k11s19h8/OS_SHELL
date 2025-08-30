/********************************************************************************************
This is a template for assignment on writing a custom Shell.

Students may change the return types and arguments of the functions given in this template,
but do not change the names of these functions.

Though use of any extra functions is not recommended, students may use new functions if they need to,
but that should not make code unnecessorily complex to read.

Students should keep names of declared variable (and any new functions) self explanatory,
and add proper comments for every logical step.

Students need to be careful while forking a new process (no unnecessory process creations)
or while inserting the single handler code (should be added at the correct places).

Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp,
as you not need to use any features for this assignment that are supported by C++ but not by C).
*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec(), pipe(), dup2()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()
#include <ctype.h>          // isspace()

#define SIZE 10000

// FORWARD DECLARATIONS
static pid_t runCommand(char **args, int should_wait);
void executeCommand(char **parsedArray);
static pid_t expand_and_runCommand(char **args, int should_wait);

void parseInput(char *input, char **parsedArray, char *delimeter)
{
    // Use simple strsep for command separators like &&, |, ##, >
    if (strcmp(delimeter, " ") != 0) {
        char *input_ptr = input;
        for (int i = 0; i < SIZE; i++) {
            parsedArray[i] = strsep(&input_ptr, delimeter);
            if (parsedArray[i] == NULL) break;
            if (strlen(parsedArray[i]) == 0) i--;
        }
        return;
    }

    //quote-aware parser 
    int arg_index = 0;
    char *current_pos = input;

    while (*current_pos != '\0' && arg_index < SIZE - 1) {
        // Skip leading whitespace before an argument
        while (isspace((unsigned char)*current_pos)) {
            current_pos++;
        }
        if (*current_pos == '\0') break;

        // Mark the beginning of the argument
        parsedArray[arg_index++] = current_pos;
        char in_quote_char = 0;

        // Scan to the end of the argument
        while (*current_pos != '\0') {
            if (*current_pos == '\'' || *current_pos == '"') {
                if (in_quote_char == 0) in_quote_char = *current_pos;
                else if (in_quote_char == *current_pos) in_quote_char = 0;
            }

            // The end of an argument is a space, but only if not in quotes
            if (isspace((unsigned char)*current_pos) && in_quote_char == 0) {
                break;
            }
            current_pos++;
        }

        // If we found the end of an argument, null-terminate it
        if (*current_pos != '\0') {
            *current_pos = '\0';
            current_pos++;
        }
    }
    parsedArray[arg_index] = NULL; // Null-terminate the array of arguments

    // Quote Removal 
    for (int i = 0; i < arg_index; i++) {
        char *arg = parsedArray[i];
        int len = strlen(arg);
        if (len >= 2 && ((arg[0] == '"' && arg[len-1] == '"') || (arg[0] == '\'' && arg[len-1] == '\''))) {
            memmove(arg, arg + 1, len - 2); // Shift string to the left
            arg[len - 2] = '\0';            // Place new null terminator
        }
    }
}

static pid_t runCommand(char **args, int should_wait)
{
	if (args[0] == NULL) return 0;
	// 'cd' is a built-in command
	if (strcmp(args[0], "cd") == 0) {
		if (args[1] == NULL) {
            printf("Shell: Incorrect command\n");
        } 
		else {
            chdir(args[1]);
        }
		return 0; // 'cd' does not create a new process
	}
	// Fork a new process for all other commands
	pid_t pid = fork();
	if (pid < 0) { exit(1); }
	if (pid == 0) { // Child process
		signal(SIGINT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		if (execvp(args[0], args) < 0) { 	// execvp returns only on error
			printf("Shell: Incorrect command\n");
		}
		exit(1);
	} else { 	// Parent process
		if (should_wait) {
            waitpid(pid, NULL, 0); 	//0 means wait for any child process
        }
	}
	return pid;
}

static pid_t expand_and_runCommand(char **args, int should_wait) {
    if (args[0] == NULL) return 0;
    
    char *new_args[SIZE] = {NULL};
    int i;
    for (i = 0; args[i] != NULL; i++) {
        char *arg = args[i];
        
        // First Pass: Calculate required memory for the new expanded argument 
        int new_len = 0;
        for (char *p = arg; *p; p++) {
            if (*p == '$') {
                if (p[1] == '$') { // PID expansion
                    char pid_str[10];
                    sprintf(pid_str, "%d", getpid());
                    new_len += strlen(pid_str);
                    p++; // Skip the second '$'
                } else if (isalnum(p[1]) || p[1] == '_') { // Environment variable
                    char var_name[100];
                    int j = 0;
                    p++; // Move to start of variable name
                    while ((isalnum(*p) || *p == '_') && j < 99) {
                        var_name[j++] = *p++;
                    }
                    var_name[j] = '\0';
                    p--; // Go back one char to not miss the next char in outer loop
                    
                    char *value = getenv(var_name);
                    if (value) new_len += strlen(value);
                } else {
                    new_len++; // Just a literal '$'
                }
            } else {
                new_len++; // Regular character
            }
        }

        // Second Pass: Building the new string
        char *expanded_arg = malloc(new_len + 1);
        char *writer = expanded_arg;
        for (char *p = arg; *p; p++) {
            if (*p == '$') {
                if (p[1] == '$') { // PID expansion
                    char pid_str[10];
                    sprintf(pid_str, "%d", getpid());
                    strcpy(writer, pid_str);
                    writer += strlen(pid_str);
                    p++; // Skip the second '$'
                } else if (isalnum(p[1]) || p[1] == '_') { // Environment variable
                    char var_name[100];
                    int j = 0;
                    p++; // Move to start of variable name
                    while ((isalnum(*p) || *p == '_') && j < 99) {
                        var_name[j++] = *p++;
                    }
                    var_name[j] = '\0';
                    p--; // Go back one char
                    
                    char *value = getenv(var_name);
                    if (value) {
                        strcpy(writer, value);
                        writer += strlen(value);
                    }
                } else {
                    *writer++ = *p; // Just a literal '$'
                }
            } else {
                *writer++ = *p; // Regular character
            }
        }
        *writer = '\0';
        new_args[i] = expanded_arg;
    }

    pid_t pid = runCommand(new_args, should_wait);

    for (int j = 0; j < i; j++) {
        free(new_args[j]);
    }
    
    return pid;
}

void executeCommand(char **parsedArray)
{
	expand_and_runCommand(parsedArray, 1); // This function runs a single command and waits for it to complete.
}

void executeSequentialCommands(char **parsedArray)
{
	for (int i = 0; parsedArray[i] != NULL; i++) {
		char *parsed[SIZE] = {NULL};
		parseInput(parsedArray[i], parsed, " ");
		executeCommand(parsed);// Waits for each command to finish before starting the next
	}
}

void executeParallelCommands(char **parsedArray)
{
	int i = 0;
	pid_t child_pids[SIZE];
	for (i = 0; parsedArray[i] != NULL; i++) {
		char *parsed[SIZE] = {NULL}; 	//to handle garbage values
		parseInput(parsedArray[i], parsed, " ");
		child_pids[i] = expand_and_runCommand(parsed, 0); // should_wait = 0
	}
	// Wait for all forked child processes to finish
	//if not waited, they become zombie processes
	for (int j = 0; j < i; j++) {
		if (child_pids[j] > 0) waitpid(child_pids[j], NULL, 0);
	}
}

void executeCommandRedirection(char **parsedArray)
{
	// This function will run a single command with output redirected to an output file
	char *command[SIZE] = {NULL}, *output_file[SIZE] = {NULL};
	parseInput(parsedArray[0], command, " ");
	parseInput(parsedArray[1], output_file, " ");
	
    if(command[0] == NULL || output_file[0] == NULL){
        printf("Shell: Incorrect command\n");
        return;
    }

	pid_t pid = fork();
	if (pid < 0) { exit(1); }
	if (pid == 0){ // Child process
		signal(SIGINT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		int fd = open(output_file[0], O_CREAT | O_WRONLY | O_TRUNC, 0644);
		if (fd < 0) { exit(1); }
		dup2(fd, STDOUT_FILENO); 	// dup2 makes stdout go to file
		close(fd);
		expand_and_runCommand(command, 1);
		exit(1);
	} 
	else { // Parent process
		waitpid(pid, NULL, 0);
	}
}

void executePipedCommands(char **parsedArray)
{
    int num_commands = 0;
    while(parsedArray[num_commands] != NULL) { num_commands++; }
    if (num_commands > 0 && (strlen(parsedArray[0]) == 0 || strlen(parsedArray[num_commands-1]) == 0)) {
        printf("Shell: Incorrect command\n");
        return;
    }

    int pipe_fds[2], in_fd = STDIN_FILENO;
    for (int i = 0; i < num_commands; i++) {
        if (pipe(pipe_fds) < 0) { exit(1); } //
        pid_t pid = fork();
        if (pid < 0) { exit(1); }
        if (pid == 0) {// Child process
			signal(SIGINT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
            if (in_fd != STDIN_FILENO) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < num_commands - 1) {
                dup2(pipe_fds[1], STDOUT_FILENO);
            }
            close(pipe_fds[0]);
            close(pipe_fds[1]);
            char *current_command[SIZE] = {NULL};
            parseInput(parsedArray[i], current_command, " ");
            if (current_command[0] == NULL || execvp(current_command[0], current_command) < 0) {
                printf("Shell: Incorrect command\n");
            }
            exit(1);
        } else {// Parent process
            if (in_fd != STDIN_FILENO) { close(in_fd); }
            close(pipe_fds[1]);
            in_fd = pipe_fds[0];
        }
    }
	if (in_fd != STDIN_FILENO){
        close(in_fd); 
    }
    for (int i = 0; i < num_commands; i++){
        wait(NULL);
    }
}

// Function to check if an operator is present outside of quotes
int is_operator_present(const char* input, const char* op) {
    char in_quote_char = 0; // 0 for no quotes, otherwise ' or "
    int op_len = strlen(op);
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] == '\'' || input[i] == '"') {// Toggle quote state
            if (in_quote_char == 0) in_quote_char = input[i];// Enter quote mode
            else if (in_quote_char == input[i]) in_quote_char = 0;
        }
        // If we are not inside quotes, check for the operator
        if (in_quote_char == 0) {
            if (strncmp(&input[i], op, op_len) == 0) return 1;// Found an unquoted operator
        }
    }
    return 0;
}

int main()
{
	char cwd[SIZE];
	char* parsedArgs[SIZE] = {NULL};
	signal(SIGINT, SIG_IGN);
  	signal(SIGTSTP, SIG_IGN);

	while(1)
	{
		if (getcwd(cwd,sizeof(cwd)) != NULL) {
            printf("%s$",cwd);
        }
		else {
            printf("shell$ ");
        }
		char* input = NULL;
		size_t size = 0;
		if (getline(&input, &size, stdin) == -1) {
			printf("\nExiting shell...\n");
			free(input);
			exit(0);
		}
		// remove newline
		input[strcspn(input, "\n")] = 0;     
        // The new parseInput handles all whitespace and empty input cases
        // Check for exit command first
        char* temp_input_for_exit_check = strdup(input);
        char* first_token[2] = {NULL, NULL};
        parseInput(temp_input_for_exit_check, first_token, " ");
        if(first_token[0] && strcmp(first_token[0], "exit") == 0){
            printf("Exiting shell...\n");
            free(temp_input_for_exit_check);
            free(input);
            exit(0);
        }
        free(temp_input_for_exit_check);
		
		if (is_operator_present(input, "&&")) {
			parseInput(input, parsedArgs, "&&");
			executeParallelCommands(parsedArgs);
		} else if (is_operator_present(input, "##")) {
			parseInput(input, parsedArgs, "##");
			executeSequentialCommands(parsedArgs);
		} else if (is_operator_present(input, "|")) {
			parseInput(input, parsedArgs, "|");
			executePipedCommands(parsedArgs);
		} else if (is_operator_present(input, ">")) {
			parseInput(input, parsedArgs, ">");
			executeCommandRedirection(parsedArgs);
		} else {
			parseInput(input, parsedArgs, " ");
			// If parsing results in no command (e.g., empty or whitespace-only line)
			if (parsedArgs[0] != NULL) {
				executeCommand(parsedArgs);
			}
		}
		free(input);
	}
	return 0;
}
