#include "shell.h"

// --- HISTORY STATE ---
#define HISTORY_SIZE 20
char* history[HISTORY_SIZE];
int history_count = 0;

void add_to_history(const char* cmdline) {
    if (history_count < HISTORY_SIZE) {
        history[history_count] = strdup(cmdline);
        history_count++;
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(cmdline);
    }
}

// --- JOB CONTROL STATE ---
#define MAX_JOBS 50
typedef struct {
    int pid;
    char cmd[256];
    int active;
} Job;

Job jobs_list[MAX_JOBS];
int job_count = 0;

// add a new process to the background list
void add_job(int pid, char* cmd) {
    if (job_count < MAX_JOBS) {
        jobs_list[job_count].pid = pid;
        strncpy(jobs_list[job_count].cmd, cmd, 255);
        jobs_list[job_count].active = 1;
        job_count++;
        printf("[Job %d] PID %d started in background\n", job_count, pid);
    }
}

// secretly reap dead background processes without freezing the shell
void reap_zombies() {
    int status;
    pid_t pid;
    // WNOHANG makes it return immediately if no children are dead
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (jobs_list[i].pid == pid && jobs_list[i].active == 1) {
                jobs_list[i].active = 0; // remove from active jobs
                printf("\n[Job %d] PID %d finished.\n", i + 1, pid);
                break;
            }
        }
    }
}

// --- BUILT-INS ---
int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL) return 0; 

    if (strcmp(arglist[0], "exit") == 0) exit(0); 
    else if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) printf("myshell: expected argument to \"cd\"\n");
        else if (chdir(arglist[1]) != 0) perror("myshell"); 
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
        // print all jobs that are still marked active
        int active_found = 0;
        for (int i = 0; i < job_count; i++) {
            if (jobs_list[i].active == 1) {
                printf("[%d] %d %s\n", i + 1, jobs_list[i].pid, jobs_list[i].cmd);
                active_found = 1;
            }
        }
        if (!active_found) printf("No active background jobs.\n");
        return 1; 
    }
    else if (strcmp(arglist[0], "history") == 0) {
        for (int i = 0; i < history_count; i++) printf("%d %s\n", i + 1, history[i]);
        return 1;
    }
    return 0;
}

// --- MAIN LOOP ---
int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        
        // CRITICAL: reap dead zombies before printing the next prompt
        reap_zombies();

        if (cmdline[0] == '\0') { free(cmdline); continue; }

        if (cmdline[0] == '!') {
            int n = atoi(&cmdline[1]);
            if (n > 0 && n <= history_count) {
                free(cmdline);
                cmdline = strdup(history[n - 1]);
                printf("%s\n", cmdline); 
            } else {
                printf("myshell: no such event: %d\n", n);
                free(cmdline);
                continue; 
            }
        }

        add_to_history(cmdline);

        // FEATURE 6: slice the input string by semicolons[cite: 13]
        char* saveptr;
        char* single_cmd = strtok_r(cmdline, ";", &saveptr);

        // loop through every chained command sequentially[cite: 13]
        while (single_cmd != NULL) {
            
            if ((arglist = tokenize(single_cmd)) != NULL) {
                
                // check if the very last argument is an ampersand (&)[cite: 13]
                int bg = 0;
                int i = 0;
                while (arglist[i] != NULL) i++;
                
                if (i > 0 && strcmp(arglist[i - 1], "&") == 0) {
                    bg = 1; // set background flag[cite: 13]
                    free(arglist[i - 1]); // wipe the & from memory
                    arglist[i - 1] = NULL; // hide it from execution
                }

                if (arglist[0] != NULL && handle_builtin(arglist) == 0) {
                    int bg_pid = -1;
                    
                    // pass the bg flag and PID catcher into execute
                    execute(arglist, bg, &bg_pid);
                    
                    // if it was a background job, add it to our list[cite: 13]
                    if (bg == 1 && bg_pid != -1) {
                        add_job(bg_pid, arglist[0]);
                    }
                }
                
                for (int j = 0; arglist[j] != NULL; j++) free(arglist[j]);
                free(arglist);
            }
            
            // grab the next command after the semicolon
            single_cmd = strtok_r(NULL, ";", &saveptr); 
        }
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
