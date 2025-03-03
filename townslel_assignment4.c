#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_ARGS 512                    // max number of arguments
#define INPUT_LENGTH 2048            // max length of command input

/*
    Fuction: parse_input()
    Args: char *input, char **args, char **input_file, char **output_file, int *background
    Description: Reads through user input and changes it into command arguments/redirection. 
*/
void parse_input(char *input, char **args, char **input_file, char **output_file, int *background) {
    char *token;
    int arg_count = 0;      // tracks arguments read
    *input_file = NULL;
    *output_file = NULL;
    *background = 0;

    token = strtok(input, " ");
    while (token != NULL) {
        // Input "<"
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token) {
                *input_file = token;
            }
        }
        // Output ">"
        else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (token) {
                *output_file = token;
            }
        }
        // Background "&"
        else if (strcmp(token, "&") == 0) {
            *background = 1;
        }
        // All other arguments
        else {
            args[arg_count++] = token;
        }
        token = strtok(NULL, " ");
    }
    args[arg_count] = NULL; 
}

// Function to execute command
void execute_command(char **args, char *input_file, char *output_file, int background) {
    pid_t pid = fork();
    int status;

    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid == 0) { // Child process
        // Handle input redirection
        if (input_file) {
            int fd = open(input_file, O_RDONLY);
            if (fd == -1) {
                perror("open input file");
                exit(1);
            }
            dup2(fd, 0);
            close(fd);
        }
        // Handle output redirection
        if (output_file) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("open output file");
                exit(1);
            }
            dup2(fd, 1);
            close(fd);
        }
        // Execute the command
        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(1);
        }
    }
    else { // Parent process
        if (background) {
            printf("Background PID: %d\n", pid);
        }
        else {
            waitpid(pid, &status, 0);
        }
    }
}

int main() {
    char input[INPUT_LENGTH];            // holds the command line input
    char *args[MAX_ARGS];                   // holds
    char *input_file, *output_file;
    int background;

    while (1) {
        printf(": ");
        fflush(stdout);
        if (!fgets(input, INPUT_LENGTH, stdin)) {
            break;
        }
        // Remove newline character
        input[strcspn(input, "\n")] = '\0';

        // Ignore empty lines and comments
        if (input[0] == '\0' || input[0] == '#') {
            continue;
        }

        // Parse input
        parse_input(input, args, &input_file, &output_file, &background);

        // Handle built-in "exit" command
        if (strcmp(args[0], "exit") == 0) {
            exit(0);
        }
        // Execute command
        execute_command(args, input_file, output_file, background);
    }
    return EXIT_SUCCESS;
}
