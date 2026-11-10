#ifndef MYSH_H
#define MYSH_H

#define MAX_ARGS 64

enum {
	BUILTIN_NONE = 0,
	BUILTIN_HANDLED = 1,
	BUILTIN_EXIT = 2
};

int handle_builtin(char **args);
void execute_with_redirection(char **args);
void execute_pipeline(char **args, int pipe_idx);

#endif
