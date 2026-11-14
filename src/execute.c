#include "shell.h"
#include <fcntl.h> // required for open() system call

int execute(char* arglist[]) {
    // 1. SCAN FOR PIPES (|) FIRST
    int pipe_idx = -1;
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            pipe_idx = i;
            break;
        }
    }

    // IF A PIPE IS FOUND, HANDLE IT AND RETURN
    if (pipe_idx != -1) {
        // split the argument list into left and right sides of the pipe
        arglist[pipe_idx] = NULL;
        char** left_args = arglist;
        char** right_args = &arglist[pipe_idx + 1];

        // create the pipe file descriptors (fd[0] is read, fd[1] is write)
        int fd[2];
        if (pipe(fd) == -1) {
            perror("myshell: pipe failed");
            return 1;
        }

        // fork the first child (runs the left command and writes to pipe)
        int pid1 = fork();
        if (pid1 == 0) { 
            dup2(fd[1], STDOUT_FILENO); // hijack stdout to point to the pipe
            close(fd[0]); // close unused read end
            close(fd[1]); // close original write end
            
            execvp(left_args[0], left_args);
            perror("myshell: command not found");
            exit(1);
        }

        // fork the second child (runs the right command and reads from pipe)
        int pid2 = fork();
        if (pid2 == 0) { 
            dup2(fd[0], STDIN_FILENO); // hijack stdin to point to the pipe
            close(fd[0]); // close original read end
            close(fd[1]); // close unused write end
            
            execvp(right_args[0], right_args);
            perror("myshell: command not found");
            exit(1);
        }

        // parent must close both pipe ends immediately so it doesn't freeze waiting
        close(fd[0]);
        close(fd[1]);
        
        // wait for both children to finish their jobs
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
        return 0;
    }

    // 2. SCAN FOR REDIRECTION (< AND >)
    int in_fd = -1, out_fd = -1;
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0) {
            // open the target file strictly for reading
            in_fd = open(arglist[i+1], O_RDONLY);
            if (in_fd < 0) { 
                perror("myshell: input file error"); 
                return 1; 
            }
            arglist[i] = NULL; // hide the '<' from execvp
        }
        else if (strcmp(arglist[i], ">") == 0) {
            // open the target file for writing (create if missing, wipe if it exists)
            out_fd = open(arglist[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0) { 
                perror("myshell: output file error"); 
                return 1; 
            }
            arglist[i] = NULL; // hide the '>' from execvp
        }
    }

    // 3. EXECUTE NORMAL OR REDIRECTED COMMAND
    int status;
    int cpid = fork();
    
    switch (cpid) {
        case -1:
            perror("myshell: fork failed");
            return 1;
        case 0: // Child process
            // if we opened an input file, hijack stdin
            if (in_fd != -1) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            // if we opened an output file, hijack stdout
            if (out_fd != -1) {
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
            
            execvp(arglist[0], arglist);
            perror("myshell: command not found");
            exit(1);
        default: // Parent process
            // parent must close its own copies of the opened files
            if (in_fd != -1) close(in_fd);
            if (out_fd != -1) close(out_fd);
            
            waitpid(cpid, &status, 0);
            return 0;
    }
}
