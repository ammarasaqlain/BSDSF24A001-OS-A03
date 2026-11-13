#include "shell.h"

// this checks if the command is a built-in. returns 1 if handled, 0 if not.
int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL) {
        return 0; 
    }

    if (strcmp(arglist[0], "exit") == 0) {
        exit(0); // kills the shell safely
    } 
    else if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            printf("myshell: expected argument to \"cd\"\n");
        } else {
            if (chdir(arglist[1]) != 0) {
                perror("myshell"); // error if folder doesn't exist
            }
        }
        return 1; 
    } 
    else if (strcmp(arglist[0], "help") == 0) {
        printf("myshell built-in commands:\n");
        printf("  cd <dir>  : change working directory\n");
        printf("  help      : show this message\n");
        printf("  jobs      : list background jobs\n");
        printf("  exit      : terminate the shell\n");
        return 1; 
    } 
    else if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1; 
    }

    return 0; // it is NOT a built-in command
}

int main() {
    char* cmdline;
    char** arglist;

    // reads input using your exact original PROMPT macro
    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        
        if ((arglist = tokenize(cmdline)) != NULL) {
            
            // intercept built-ins before executing external commands
            if (handle_builtin(arglist) == 0) {
                execute(arglist);
            }

            // free the memory allocated by tokenize() exactly like your og code
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
