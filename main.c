// Author: Tornike Maridashvili
// main.c - Implements a command-line shell supporting a REPL loop, built-in commands 
// (cd, exit), input/output redirection (<, >), and a single command pipeline (|) 
//
#include "mysh.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
	char *line = NULL;
	size_t bufsize = 0;
	char *args[MAX_ARGS];
	struct sigaction sa;

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("mysh: sigaction failed");
		exit(EXIT_FAILURE);
	}

	while (1) {
		int i = 0;
		char *token;
		int builtin_status;
		int pipe_idx = -1;
		int pipe_count = 0;

		printf("mysh> ");
		fflush(stdout);

		if (getline(&line, &bufsize, stdin) == -1) {
			printf("\n");
			break;
		}

		line[strcspn(line, "\n")] = 0;
		if (strlen(line) == 0)
			continue;

		token = strtok(line, " \t");
		while (token != NULL && i < MAX_ARGS - 1) {
			args[i++] = token;
			token = strtok(NULL, " \t");
		}
		args[i] = NULL;

		if (args[0] == NULL)
			continue;

		builtin_status = handle_builtin(args);
		if (builtin_status == BUILTIN_EXIT) {
			free(line);
			exit(0);
		}
		if (builtin_status == BUILTIN_HANDLED)
			continue;

		for (int j = 0; args[j] != NULL; j++) {
			if (strcmp(args[j], "|") == 0) {
				pipe_count++;
				pipe_idx = j;
			}
		}

		if (pipe_count > 1) {
			fprintf(stderr, "mysh: only one pipe is supported\n");
			continue;
		}

		if (pipe_count == 1) {
			execute_pipeline(args, pipe_idx);
		} else {
			pid_t pid = fork();

			if (pid < 0) {
				perror("mysh: fork failed");
			} else if (pid == 0) {
				struct sigaction sa_child;

				sa_child.sa_handler = SIG_DFL;
				sigemptyset(&sa_child.sa_mask);
				sa_child.sa_flags = 0;
				sigaction(SIGINT, &sa_child, NULL);
				execute_with_redirection(args);
			} else {
				int status;
				waitpid(pid, &status, 0);
			}
		}
	}

	free(line);
	return 0;
}
