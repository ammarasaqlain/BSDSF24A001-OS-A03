#include "shell.h"

// define the max history size required by the assignment
#define HISTORY_SIZE 20

// the data structure to store the commands
char* history[HISTORY_SIZE];
int history_count = 0;

// custom function to push commands into the array
void add_to_history(const char* cmdline) {
    if (history_count < HISTORY_SIZE) {
        history[history_count] = strdup(cmdline);
        history_count++;
    } else {
        // if the array is full, free the oldest command and shift everything down
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        // slap the newest command at the very end
        history[HISTORY_SIZE - 1] = strdup(cmdline);
    }
}

// your built-in interceptor, now upgraded with the history command
int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL) {
        return 0; 
    }

    if (strcmp(arglist[0], "exit") == 0) {
        exit(0); 
    } 
    else if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            printf("myshell: expected argument to \"cd\"\n");
        } else {
            if (chdir(arglist[1]) != 0) {
                perror("myshell"); 
            }
        }
        return 1; 
    } 
    else if (strcmp(arglist[0], "help") == 0) {
        printf("myshell built-in commands:\n");
        printf("  cd <dir>  : change working directory\n");
        printf("  help      : show this message\n");
        printf("  jobs      : list background jobs\n");
        printf("  history   : show command history\n");
        printf("  exit      : terminate the shell\n");
        return 1; 
    } 
    else if (strcmp(arglist[0], "jobs") == 0) {
        printf("Job control not yet implemented.\n");
        return 1; 
    }
    else if (strcmp(arglist[0], "history") == 0) {
        // loop through storage and print each command preceded by its line number
        for (int i = 0; i < history_count; i++) {
            printf("%d %s\n", i + 1, history[i]);
        }
        return 1;
    }

    return 0; // NOT a built-in command
}

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        
        // ignore empty presses so they don't clog the history
        if (cmdline[0] == '\0') {
            free(cmdline);
            continue;
        }

        // CRITICAL: catch the !n execution feature before tokenization
        if (cmdline[0] == '!') {
            // parse the number n that follows the !
            int n = atoi(&cmdline[1]);
            
            // make sure the number is valid and in bounds
            if (n > 0 && n <= history_count) {
                free(cmdline);
                // retrieve the n-th command string and replace the command line
                cmdline = strdup(history[n - 1]);
                printf("%s\n", cmdline); // echo the retrieved command to the user
            } else {
                printf("myshell: no such event: %d\n", n);
                free(cmdline);
                continue; // skip the rest of the loop
            }
        }

        // instantly add the successfully read (or retrieved) command to history storage
        add_to_history(cmdline);

        // proceed to tokenize and execute as normal
        if ((arglist = tokenize(cmdline)) != NULL) {
            
            if (handle_builtin(arglist) == 0) {
                execute(arglist);
            }

            // memory cleanup
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
