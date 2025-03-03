#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_ARGS 512                            // max number of arguments
#define INPUT_LENGTH 2048                       // max length of command input
#define MAX_BG_PROCESSES 100                    // Store up to 100 background PIDs

pid_t background_pids[MAX_BG_PROCESSES] = {0};  // Array to track background PIDs
int last_status = 0;                            // Store last foreground process status
int foreground_only_mode = 0;                   // Store whether we're in foreground_only_mode
volatile sig_atomic_t sigtstp_flag = 0;         // flag to track if SIGTSTP was received

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
        // ignore SIGTSTP in foreground and background
        struct sigaction sa_SIGTSTP_child = {0};
        sa_SIGTSTP_child.sa_handler = SIG_IGN; // Ignore SIGTSTP
        sigfillset(&sa_SIGTSTP_child.sa_mask);
        sa_SIGTSTP_child.sa_flags = 0;
        sigaction(SIGTSTP, &sa_SIGTSTP_child, NULL);

        // restore SIGINT for foreground processes
        if (!background) {
            struct sigaction sa_SIGINT_child = {0};
            sa_SIGINT_child.sa_handler = SIG_DFL; 
            sigfillset(&sa_SIGINT_child.sa_mask);
            sa_SIGINT_child.sa_flags = 0;
            sigaction(SIGINT, &sa_SIGINT_child, NULL);
        }

        // Handle Input
        if (input_file) {
            int file = open(input_file, O_RDONLY);
            if (file == -1) {
                fprintf(stderr, "cannot open %s for input\n", input_file);
                fflush(stdout);
                exit(1);
            }
            dup2(file, 0);
            close(file);
        } else if (background) {  
            // Redirect background input to /dev/null
            int devnull = open("/dev/null", O_RDONLY);
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        }

        // Handle Output
        if (output_file) {
            int file = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (file == -1) {
                fprintf(stderr, "cannot open %s for output\n", output_file);
                fflush(stdout);
                exit(1);
            }
            dup2(file, 1);
            close(file);
        } else if (background) {  
            // Redirect background output to /dev/null
            int devnull = open("/dev/null", O_WRONLY);
            dup2(devnull, STDOUT_FILENO);
            close(devnull);
        }

        // Execute the command
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "%s: no such file or directory\n", args[0]);
            fflush(stdout);
            exit(1);
        }
    }
    else { 
        if (background && !foreground_only_mode) {
            printf("background pid is %d\n", pid);
            fflush(stdout);

            // Store background PID in array
            for (int i = 0; i < MAX_BG_PROCESSES; i++) {
                if (background_pids[i] == 0) {
                    background_pids[i] = pid;
                    break;
                }
            }
        }
        else {
            waitpid(pid, &status, 0);

            if (WIFSIGNALED(status)) {
                int term_signal = WTERMSIG(status);
                printf("terminated by signal %d\n", term_signal);
                fflush(stdout);
                last_status = term_signal;
            } else if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            }
        }
    }
}

/*
    Function: handle_SIGINT()
    Description: Do nothing when SIGINT is called
*/
void handle_SIGINT(int signo) {
    // Do nothing! Yay!
}

/*
    Function: handle_SIGTSTP()
    Description: Set flag for foreground-only mode 
*/
void handle_SIGTSTP(int signo) {
    sigtstp_flag = 1;
}

/*
    Function: check_background_processes()
    Description: Check background processes and print when status is completed
*/
void check_background_processes() {
    int status;
    pid_t pid;

    for (int i = 0; i < MAX_BG_PROCESSES; i++) {
        if (background_pids[i] != 0) {
            pid = waitpid(background_pids[i], &status, WNOHANG);
            if (pid > 0) {  
                if (WIFSIGNALED(status)) {
                    printf("background pid %d is done: terminated by signal %d\n", pid, WTERMSIG(status));
                } else if (WIFEXITED(status)) {
                    printf("background pid %d is done: exit value %d\n", pid, WEXITSTATUS(status));
                }
                fflush(stdout);
                background_pids[i] = 0;  
            }
        }
    }
}

/*
    Function: main()
*/
int main() {
    // handle SIGINT
    struct sigaction sa_SIGINT = {0};
    sa_SIGINT.sa_handler = handle_SIGINT; 
    sigfillset(&sa_SIGINT.sa_mask);
    sa_SIGINT.sa_flags = 0;
    sigaction(SIGINT, &sa_SIGINT, NULL);

    // handle SIGTSTP
    struct sigaction sa_SIGTSTP = {0};
    sa_SIGTSTP.sa_handler = handle_SIGTSTP;
    sigfillset(&sa_SIGTSTP.sa_mask);
    sa_SIGTSTP.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_SIGTSTP, NULL);

    // variables
    char input[INPUT_LENGTH];               // holds the user input
    char *args[MAX_ARGS];                   
    char *input_file, *output_file;
    int background;                         // bool if the command runs in the background

    while (1) {
        check_background_processes();  

        // Check if SIGTSTP was received
        if (sigtstp_flag) {
            if (foreground_only_mode == 0) {
                write(STDOUT_FILENO, "\nEntering foreground-only mode (& is now ignored)\n", 50);
                foreground_only_mode = 1;
            } else {
                write(STDOUT_FILENO, "\nExiting foreground-only mode\n", 30);
                foreground_only_mode = 0;
            }
            sigtstp_flag = 0; // Reset flag
        }
        
        // text input 
        printf(": ");
        fflush(stdout);
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
