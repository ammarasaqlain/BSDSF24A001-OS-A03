#include "shell.h"
#include <fcntl.h>

// updated to accept the background flag (bg) and a pointer to send the PID back (bg_pid)
int execute(char* arglist[], int bg, int* bg_pid) {
    
    // 1. PIPES
    int pipe_idx = -1;
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            pipe_idx = i;
            break;
        }
    }

    if (pipe_idx != -1) {
        arglist[pipe_idx] = NULL;
        char** left_args = arglist;
        char** right_args = &arglist[pipe_idx + 1];

        int fd[2];
        if (pipe(fd) == -1) { perror("myshell: pipe failed"); return 1; }

        int pid1 = fork();
        if (pid1 == 0) {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]); close(fd[1]);
            execvp(left_args[0], left_args);
            perror("myshell: command not found"); exit(1);
        }

        int pid2 = fork();
        if (pid2 == 0) {
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]); close(fd[1]);
            execvp(right_args[0], right_args);
            perror("myshell: command not found"); exit(1);
        }

        close(fd[0]); close(fd[1]);

        // if background is true, track the last process of the pipe and DO NOT wait
        if (bg) {
            *bg_pid = pid2; 
        } else {
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
        }
        return 0;
    }

    // 2. REDIRECTION
    int in_fd = -1, out_fd = -1;
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0) {
            in_fd = open(arglist[i+1], O_RDONLY);
            if (in_fd < 0) { perror("myshell: input file error"); return 1; }
            arglist[i] = NULL;
        }
        else if (strcmp(arglist[i], ">") == 0) {
            out_fd = open(arglist[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0) { perror("myshell: output file error"); return 1; }
            arglist[i] = NULL;
        }
    }

    // 3. EXECUTION
    int status;
    int cpid = fork();

    switch (cpid) {
        case -1:
            perror("myshell: fork failed");
            return 1;
        case 0:
            if (in_fd != -1) { dup2(in_fd, STDIN_FILENO); close(in_fd); }
            if (out_fd != -1) { dup2(out_fd, STDOUT_FILENO); close(out_fd); }
            execvp(arglist[0], arglist);
            perror("myshell: command not found");
            exit(1);
        default:
            if (in_fd != -1) close(in_fd);
            if (out_fd != -1) close(out_fd);

            // FEATURE 6: if background, pass PID back and skip waitpid
            if (bg) {
                *bg_pid = cpid;
            } else {
                waitpid(cpid, &status, 0);
            }
            return 0;
    }
}
