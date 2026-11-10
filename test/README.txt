mysh tests (Midterm Project)
============================

Run from the project root or test directory:

  ./test/test.sh

The script builds `mysh` with `make` and runs automated checks based on
Midterm proj.pdf requirements:

- Single commands: prompt/execute/wait behavior with arguments.
- Redirection: `<` and `>` on commands, including `sort < in | uniq -c > out`.
- Pipe: exactly one pipe between two commands; rejects more than one pipe.
- Builtins/errors: `cd` (including missing argument), `exit`, EOF, missing
  command / file errors reported as `mysh: ...` to stderr while shell continues.
- Basic FD sanity: runs `ls /proc/self/fd` under mysh (manual inspection still
  recommended; mysh has no quote parsing, so tests avoid quoted args).
- Pipeline hang proxy: shell continues after a pipeline.

Manual checks recommended by the spec:
- Ctrl-C kills the running command but not `mysh` (interactive).
- No zombies left behind (`ps`, observe no defunct children during/after use).
- No extra inherited file descriptors: verify commands do not inherit mysh's
  internal fds (inspect `/proc/<pid>/fd` or command fd listings as appropriate).
- If a pipeline hangs, inspect whether any process still holds a pipe end open.

Notes:
- Tests assume `<`, `>`, and `|` are space-separated (per spec).
- The script uses `/tmp/opencode/mysh-test-*` for temporary files.
