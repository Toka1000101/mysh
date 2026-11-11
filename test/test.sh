#!/usr/bin/env bash
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MYSH="$ROOT/mysh"
TMP="/tmp/opencode/mysh-test-$$"
PASS=0
FAIL=0

mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT

pass() { echo "PASS: $1"; PASS=$((PASS+1)); }
fail() { echo "FAIL: $1"; echo "  $2"; FAIL=$((FAIL+1)); }

assert_eq() {
	local name="$1" expected="$2" actual="$3"
	if [ "$expected" = "$actual" ]; then pass "$name"; else fail "$name" "expected: [$expected] actual: [$actual]"; fi
}

assert_contains() {
	local name="$1" needle="$2" hay="$3"
	if printf '%s' "$hay" | grep -Fq -- "$needle"; then pass "$name"; else fail "$name" "missing [$needle] in [$hay]"; fi
}

assert_file_eq() {
	local name="$1" expected="$2" file="$3"
	local actual
	actual="$(cat "$file" 2>/dev/null || true)"
	assert_eq "$name" "$expected" "$actual"
}

echo "Building mysh..."
make -C "$ROOT" >/dev/null || { echo "Build failed"; exit 1; }

# 1. Single command (20)
out="$(printf 'echo hello world\nexit\n' | "$MYSH" 2>"$TMP/err")"
assert_contains "single command output" "hello world" "$out"

out="$(printf '/bin/echo alpha beta\nexit\n' | "$MYSH" 2>"$TMP/err")"
assert_contains "single command with path/args" "alpha beta" "$out"

# 2. Redirection (20)
printf 'apple\npear\nfig\n' > "$TMP/in.txt"
out="$(printf 'cat < '"$TMP/in.txt"$'\nexit\n' | "$MYSH" 2>"$TMP/err")"
assert_contains "input redirection" "apple" "$out"

printf 'echo redirected > '"$TMP/out.txt"$'\nexit\n' | "$MYSH" >"$TMP/stdout" 2>"$TMP/err"
assert_file_eq "output redirection file" "redirected" "$TMP/out.txt"

printf 'apple\npear\napple\nfig\npear\napple\n' > "$TMP/fruits.txt"
printf 'sort < '"$TMP/fruits.txt"' | uniq -c > '"$TMP/counts.txt"$'\nexit\n' | "$MYSH" >"$TMP/stdout" 2>"$TMP/err"
# normalize spacing in uniq -c output for comparison
expected_counts="$(sort < "$TMP/fruits.txt" | uniq -c)"
actual_counts="$(cat "$TMP/counts.txt")"
if [ "$expected_counts" = "$actual_counts" ]; then
	pass "redirection with pipe (sort < in | uniq -c > out)"
else
	fail "redirection with pipe (sort < in | uniq -c > out)" "expected: [$expected_counts] actual: [$actual_counts]"
fi

# 3. Pipe (30)
out="$(printf 'echo hello | cat\nexit\n' | "$MYSH" 2>"$TMP/err")"
assert_contains "single pipe" "hello" "$out"

printf 'a\nb\n' > "$TMP/lines.txt"
out="$(printf 'cat '"$TMP/lines.txt"' | wc -l\nexit\n' | "$MYSH" 2>"$TMP/err")"
# wc -l output may have spaces; accept "2"
if printf '%s' "$out" | grep -Eq '[[:space:]]2([[:space:]]|$)'; then pass "pipe data flow"; else fail "pipe data flow" "output: [$out]"; fi

# 4. cd/exit and errors (15)
mkdir -p "$TMP/subdir"
printf 'marker\n' > "$TMP/subdir/marker.txt"
out="$(printf 'cd '"$TMP/subdir"$'\ncat marker.txt\nexit\n' | "$MYSH" 2>"$TMP/err")"
assert_contains "cd builtin affects subsequent command" "marker" "$out"

# A builtin used as a pipeline stage must run in that child; the other
# stage must still execute (regression: cd .. | ls skipped ls).
out="$(printf 'cd .. | echo pipeline_after_builtin\nexit\n' | "$MYSH" 2>"$TMP/err")"
assert_contains "builtin in pipe does not skip other stage" "pipeline_after_builtin" "$out"

printf 'cd\nexit\n' | "$MYSH" >"$TMP/stdout" 2>"$TMP/err"
assert_contains "cd missing argument to stderr" "mysh: cd: missing argument" "$(cat "$TMP/err")"

printf 'nosuchcmd_xyz\nexit\n' | "$MYSH" >"$TMP/stdout" 2>"$TMP/err"
assert_contains "missing command error prefix" "mysh:" "$(cat "$TMP/err")"
assert_contains "missing command name/message" "nosuchcmd_xyz" "$(cat "$TMP/err")"
# shell should continue to process exit
assert_contains "shell continues after error" "mysh>" "$(cat "$TMP/stdout")"

printf 'cat < '"$TMP/no_such_file_$$"$'\nexit\n' | "$MYSH" >"$TMP/stdout" 2>"$TMP/err"
assert_contains "bad input redirection reports mysh error" "mysh:" "$(cat "$TMP/err")"

printf 'ls | sort | head\nexit\n' | "$MYSH" >"$TMP/stdout" 2>"$TMP/err"
assert_contains "multiple pipes rejected" "only one pipe is supported" "$(cat "$TMP/err")"

# exit builtin ends shell (EOF not required after)
out="$(printf 'exit\n' | "$MYSH" 2>"$TMP/err"; echo "---$?---")"
assert_contains "exit ends shell cleanly" "---0---" "$out"

# EOF ends shell
out="$(printf 'echo eof' | "$MYSH" 2>"$TMP/err"; echo "---$?---")"
assert_contains "EOF ends shell" "---0---" "$out"

# 5. Prompt/behavior basics
out="$(printf 'echo hi\nexit\n' | "$MYSH" 2>"$TMP/err")"
assert_contains "prompt printed" "mysh> " "$out"

# 6. No extra inherited FDs (basic check; spec requires no extra inherited FDs)
# mysh has no quoting parser, so run a simple command directly.
out="$(printf 'ls /proc/self/fd\nexit\n' | "$MYSH" 2>"$TMP/err" | tr -d '\r')"
if printf '%s' "$out" | grep -Eq '(^|mysh> |[[:space:]])0($|[[:space:]])'; then
	pass "fd listing command runs (manual inspection recommended for leaks)"
else
	fail "fd listing command runs" "output: [$out]"
fi

# 7. No zombies / completes (functional proxy): pipeline and commands return control
out="$(printf 'echo a | cat\necho done\nexit\n' | "$MYSH" 2>"$TMP/err")"
assert_contains "pipeline does not hang; shell continues" "done" "$out"

echo
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
