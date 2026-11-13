#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shell.h"

char* read_cmd(char* prompt, FILE* fp) {
    // readline automatically prints the prompt and reads the user input
    // it completely ignores the 'fp' (file pointer) since it reads straight from the terminal
    char* cmdline = readline(prompt);

    // if the user presses Ctrl+D (EOF), readline returns NULL.
    if (cmdline == NULL) {
        return NULL;
    }

    // if the command is not empty, we feed it to the history list.
    if (cmdline[0] != '\0') {
        add_history(cmdline);
    }

    return cmdline;
}

char** tokenize(char* cmdline) {
    // Edge case: empty command line
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // Skip leading whitespace

        if (*cp == '\0') break; // Line was only whitespace

        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) {
            len++;
        }
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) { // No arguments were parsed
        for(int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}
