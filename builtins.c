// Tornike Maridashivli
// handle buiilt-in commands, cd and exit
#include "mysh.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int handle_builtin(char **args)
{
	if (strcmp(args[0], "exit") == 0)
		return BUILTIN_EXIT;

	if (strcmp(args[0], "cd") == 0) {
		if (args[1] == NULL)
			fprintf(stderr, "mysh: cd: missing argument\n");
		else if (chdir(args[1]) != 0)
			perror("mysh: cd");
		return BUILTIN_HANDLED;
	}

	return BUILTIN_NONE;
}
