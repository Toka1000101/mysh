CC = gcc
CFLAGS = -Wall -Wextra
SRCS = main.c builtins.c execute.c
OBJS = $(SRCS:.c=.o)

mysh: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c mysh.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f mysh $(OBJS)


