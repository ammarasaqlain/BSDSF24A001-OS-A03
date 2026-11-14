#include "shell.h"
#include <readline/readline.h>

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
        for (int i = 1; i < HISTORY_SIZE; i++) history[i - 1] = history[i];
        history[HISTORY_SIZE - 1] = strdup(cmdline);
    }
}

// --- JOB CONTROL STATE ---
#define MAX_JOBS 50
typedef struct { int pid; char cmd[256]; int active; } Job;
Job jobs_list[MAX_JOBS];
int job_count = 0;

void add_job(int pid, char* cmd) {
    if (job_count < MAX_JOBS) {
        jobs_list[job_count].pid = pid;
        strncpy(jobs_list[job_count].cmd, cmd, 255);
        jobs_list[job_count].active = 1;
        job_count++;
        printf("[Job %d] PID %d started in background\n", job_count, pid);
    }
}

void reap_zombies() {
    int status; pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < job_count; i++) {
            if (jobs_list[i].pid == pid && jobs_list[i].active == 1) {
                jobs_list[i].active = 0; 
                printf("\n[Job %d] PID %d finished.\n", i + 1, pid);
                break;
            }
        }
    }
}

// =========================================================
// FEATURE 8: VARIABLE STORAGE STATE
// =========================================================
#define MAX_VARS 100
typedef struct { char name[64]; char value[256]; } ShellVar;
ShellVar var_list[MAX_VARS];
int var_count = 0;

void set_var(char* name, char* value) {
    // update it if it already exists
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_list[i].name, name) == 0) {
            strncpy(var_list[i].value, value, 255);
            return;
        }
    }
    // otherwise add a brand new variable
    if (var_count < MAX_VARS) {
        strncpy(var_list[var_count].name, name, 63);
        strncpy(var_list[var_count].value, value, 255);
        var_count++;
    }
}

char* get_var(char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_list[i].name, name) == 0) return var_list[i].value;
    }
    return ""; // return empty string if the variable does not exist
}

// --- BUILT-INS ---
int handle_builtin(char** arglist) {
    if (arglist == NULL || arglist[0] == NULL) return 0; 

    // FEATURE 8: parse VAR=value assignments (must have no spaces)
    char* eq_pos = strchr(arglist[0], '=');
    if (eq_pos != NULL && arglist[1] == NULL) {
        *eq_pos = '\0'; // physically split the string at the '=' sign
        char* var_name = arglist[0];
        char* var_val = eq_pos + 1;
        
        // automatically strip quotation marks if the user types MSG="hello"
        if (var_val[0] == '"') {
            var_val++; 
            int len = strlen(var_val);
            if (len > 0 && var_val[len-1] == '"') var_val[len-1] = '\0';
        }
        
        set_var(var_name, var_val); // save it to memory
        return 1;
    }

    if (strcmp(arglist[0], "exit") == 0) exit(0); 
    else if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) printf("myshell: expected argument to \"cd\"\n");
        else if (chdir(arglist[1]) != 0) perror("myshell"); 
        return 1; 
    } 
    else if (strcmp(arglist[0], "help") == 0) {
        printf("myshell built-in commands:\n  cd <dir>\n  help\n  jobs\n  history\n  set\n  exit\n");
        return 1; 
    } 
    else if (strcmp(arglist[0], "jobs") == 0) {
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
    // FEATURE 8: the set command prints all saved variables
    else if (strcmp(arglist[0], "set") == 0) {
        for (int i = 0; i < var_count; i++) {
            printf("%s=%s\n", var_list[i].name, var_list[i].value);
        }
        return 1;
    }
    else if (strcmp(arglist[0], "history") == 0) {
        for (int i = 0; i < history_count; i++) printf("%d %s\n", i + 1, history[i]);
        return 1;
    }
    return 0;
}

// --- LINE PROCESSOR ---
void process_single_line(char* line) {
    char* saveptr;
    char* line_copy = strdup(line); 
    char* single_cmd = strtok_r(line_copy, ";", &saveptr);

    while (single_cmd != NULL) {
        char** arglist;
        if ((arglist = tokenize(single_cmd)) != NULL) {
            
            // FEATURE 8: Variable Expansion Step
            // scan every token. if it starts with $, swap it with its saved value[cite: 11]
            for (int k = 0; arglist[k] != NULL; k++) {
                if (arglist[k][0] == '$') {
                    char* val = get_var(arglist[k] + 1); // get text after the $
                    free(arglist[k]); 
                    arglist[k] = strdup(val); // inject the saved value[cite: 11]
                }
            }

            int bg = 0, i = 0;
            while (arglist[i] != NULL) i++;
            
            if (i > 0 && strcmp(arglist[i - 1], "&") == 0) {
                bg = 1; free(arglist[i - 1]); arglist[i - 1] = NULL;
            }

            if (arglist[0] != NULL && handle_builtin(arglist) == 0) {
                int bg_pid = -1;
                execute(arglist, bg, &bg_pid);
                if (bg == 1 && bg_pid != -1) add_job(bg_pid, arglist[0]);
            }
            for (int j = 0; arglist[j] != NULL; j++) free(arglist[j]);
            free(arglist);
        }
        single_cmd = strtok_r(NULL, ";", &saveptr); 
    }
    free(line_copy);
}

// --- MAIN LOOP ---
int main() {
    char* cmdline;
    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        reap_zombies();

        if (cmdline[0] == '\0') { free(cmdline); continue; }

        if (cmdline[0] == '!') {
            int n = atoi(&cmdline[1]);
            if (n > 0 && n <= history_count) {
                free(cmdline); cmdline = strdup(history[n - 1]); printf("%s\n", cmdline); 
            } else {
                printf("myshell: no such event: %d\n", n); free(cmdline); continue; 
            }
        }

        add_to_history(cmdline);

        if (strncmp(cmdline, "if ", 3) == 0) {
            char if_cond[256];
            strcpy(if_cond, cmdline + 3); 

            char then_cmds[20][256]; int then_count = 0;
            char else_cmds[20][256]; int else_count = 0;
            int in_else = 0;

            while (1) {
                char* line = readline("> "); 
                if (!line) break;
                char* trimmed = line;
                while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
                if (strcmp(trimmed, "fi") == 0) { free(line); break; }
                if (strcmp(trimmed, "then") == 0) { free(line); continue; }
                if (strcmp(trimmed, "else") == 0) { in_else = 1; free(line); continue; }

                if (trimmed[0] != '\0') {
                    if (in_else) strcpy(else_cmds[else_count++], trimmed);
                    else strcpy(then_cmds[then_count++], trimmed);
                }
                free(line);
            }

            char** if_args = tokenize(if_cond);
            int exit_code = 1; 
            if (if_args != NULL) {
                
                // FEATURE 8: Expand variables inside the 'if' condition too[cite: 11]
                for (int k = 0; if_args[k] != NULL; k++) {
                    if (if_args[k][0] == '$') {
                        char* val = get_var(if_args[k] + 1);
                        free(if_args[k]); 
                        if_args[k] = strdup(val);
                    }
                }

                int bg_pid = -1;
                exit_code = execute(if_args, 0, &bg_pid); 
                for (int j = 0; if_args[j] != NULL; j++) free(if_args[j]);
                free(if_args);
            }

            if (exit_code == 0) {
                for (int i = 0; i < then_count; i++) process_single_line(then_cmds[i]);
            } else {
                for (int i = 0; i < else_count; i++) process_single_line(else_cmds[i]);
            }

            free(cmdline);
            continue; 
        }

        process_single_line(cmdline);
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
