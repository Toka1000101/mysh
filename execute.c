// Tornike Maridashvili
// execute.c - handle redirection and pipe
// execute_with_redirection(char **args) - handles '<' and '>'
// void execute_pipeline(char **args, int pipe_idx) -  handles '|'
// reset_sigint(void) - resets default behaviour of shell
#include "mysh.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void reset_sigint(void) {
	struct sigaction sa_child;

	sa_child.sa_handler = SIG_DFL;
	sigemptyset(&sa_child.sa_mask);
	sa_child.sa_flags = 0;
	sigaction(SIGINT, &sa_child, NULL);
}

void execute_with_redirection(char **args) {
	int i = 0;
	char *clean_args[MAX_ARGS];
	int clean_i = 0;

	while (args[i] != NULL) {
		if (strcmp(args[i], "<") == 0) {
			int fd;

			if (args[i + 1] == NULL) {
				fprintf(stderr, "mysh: syntax error near unexpected token 'newline'\n");
				exit(EXIT_FAILURE);
			}
			fd = open(args[i + 1], O_RDONLY);
			if (fd < 0) {
				fprintf(stderr, "mysh: %s: %s\n", args[i + 1], strerror(errno));
				exit(EXIT_FAILURE);
			}
			if (dup2(fd, STDIN_FILENO) < 0) {
				perror("mysh: dup2 failed");
				close(fd);
				exit(EXIT_FAILURE);
			}
			close(fd);
			i += 2;
		} else if (strcmp(args[i], ">") == 0) {
			int fd;

			if (args[i + 1] == NULL) {
				fprintf(stderr, "mysh: syntax error near unexpected token 'newline'\n");
				exit(EXIT_FAILURE);
			}
			fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (fd < 0) {
				fprintf(stderr, "mysh: %s: %s\n", args[i + 1], strerror(errno));
				exit(EXIT_FAILURE);
			}
			if (dup2(fd, STDOUT_FILENO) < 0) {
				perror("mysh: dup2 failed");
				close(fd);
				exit(EXIT_FAILURE);
			}
			close(fd);
			i += 2;
		} else {
			clean_args[clean_i++] = args[i];
			i++;
		}
	}
	clean_args[clean_i] = NULL;

	if (clean_args[0] == NULL)
		exit(EXIT_SUCCESS);

	execvp(clean_args[0], clean_args);
	fprintf(stderr, "mysh: %s: %s\n", clean_args[0], strerror(errno));
	exit(EXIT_FAILURE);
}

void execute_pipeline(char **args, int pipe_idx) {
	char **cmd1;
	char **cmd2;
	int pipefd[2];
	pid_t pid1;
	pid_t pid2;

	args[pipe_idx] = NULL;
	cmd1 = args;
	cmd2 = &args[pipe_idx + 1];

	if (cmd1[0] == NULL || cmd2[0] == NULL) {
		fprintf(stderr, "mysh: syntax error near unexpected token `|'\n");
		return;
	}

	if (pipe(pipefd) < 0) {
		perror("mysh: pipe failed");
		return;
	}

	pid1 = fork();
	if (pid1 < 0) {
		perror("mysh: fork failed");
		close(pipefd[0]);
		close(pipefd[1]);
		return;
	}
	if (pid1 == 0) {
		reset_sigint();
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);
		execute_with_redirection(cmd1);
	}

	pid2 = fork();
	if (pid2 < 0) {
		perror("mysh: fork failed");
		close(pipefd[0]);
		close(pipefd[1]);
		waitpid(pid1, NULL, 0);
		return;
	}
	if (pid2 == 0) {
		reset_sigint();
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		close(pipefd[1]);
		execute_with_redirection(cmd2);
	}

	close(pipefd[0]);
	close(pipefd[1]);
	waitpid(pid1, NULL, 0);
	waitpid(pid2, NULL, 0);
}
