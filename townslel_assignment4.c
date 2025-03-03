#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_ARGS 512                 // max number of arguments
#define INPUT_LENGTH 2048            // max length of command input

int last_status = 0;                // Store last foreground process status

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
/*
    Function: execute_command()
    Args: char **args, char *input_file, char *output_file, int background
    Description: Executes commands from the arguements
*/
void execute_command(char **args, char *input_file, char *output_file, int background) {
    pid_t pid = fork();         //fork a child process and store process ID
    int status;

    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    else if (pid == 0) { 
        // Handle Input
        if (input_file) {
            int file = open(input_file, O_RDONLY);
            if (file == -1) {
                fprintf(stderr, "cannot open %s for input\n", input_file);
                exit(1);
            }
            dup2(file, 0);
            close(file);
        }
        // Handle Output
        if (output_file) {
            int file = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (file == -1) {
                fprintf(stderr, "cannot open %s for output\n", output_file);
                exit(1);
            }
            dup2(file, 1);
            close(file);
        }
        // Execute the command
        if (execvp(args[0], args) == -1) {
            perror("execvp");
            fprintf(stderr, "%s: no such file or directory", output_file);
            exit(1);
        }
    }
    else { 
        if (background) {
            printf("background pid is %d\n", pid);
        }
        else {
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            } 
            //else if (WIFSIGNALED(status)) {
            //    last_status = WTERMSIG(status);
            //    printf("Terminated by signal %d\n", last_status);
            //}
        }
    }
}

int main() {
    char input[INPUT_LENGTH];               // holds the user input
    char *args[MAX_ARGS];                   
    char *input_file, *output_file;
    int background;                         // bool if the command runs in the background

    while (1) {
        printf(": ");
        //fflush(stdout);
        if (!fgets(input, INPUT_LENGTH, stdin)) {
            break;
        }
        // remove newline
        input[strcspn(input, "\n")] = '\0';

        // ignore empty lines & comments
        if (input[0] == '\0' || input[0] == '#') {
            continue;
        }

        parse_input(input, args, &input_file, &output_file, &background);

        // exit
        if (strcmp(args[0], "exit") == 0) {
            exit(0);
        }
        // cd
        else if (strcmp(args[0], "cd") == 0){
            if (args[1] == NULL){
                chdir(getenv("HOME"));              // changes to the directory specified in the HOME environment variable
            } else {
                if (chdir(args[1]) != 0){
                    perror("cd");
                }
            }
        }
        // status
        else if (strcmp(args[0], "status") == 0) {
            printf("exit status: %d\n", last_status);
        }

        // Execute 
        else{
            execute_command(args, input_file, output_file, background);
        }
           
    }
    return EXIT_SUCCESS;
}
