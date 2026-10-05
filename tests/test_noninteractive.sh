#!/usr/bin/env bash
set -euo pipefail
export TERM="${TERM:-xterm}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
run_shell() { timeout 8s ./shellforge; }

out="$(printf 'echo hello\nexit\n' | run_shell)"
grep -q 'hello' <<<"$out"

out="$(printf 'echo hello | cat\nexit\n' | run_shell)"
grep -q 'hello' <<<"$out"

rm -f sf_test_output.txt
out="$(printf 'echo hello > sf_test_output.txt\ncat sf_test_output.txt\nexit\n' | run_shell)"
grep -q 'hello' <<<"$out"
test "$(cat sf_test_output.txt)" = "hello"
rm -f sf_test_output.txt

out="$(printf 'echo $HOME\nexit\n' | run_shell)"
grep -q "$HOME" <<<"$out"

out="$(printf 'sleep 0.1 &\nsleep 0.3\nexit\n' | run_shell)"
grep -q 'Done' <<<"$out"

out="$(printf 'sleep 0.5 &\njobs\nexit\n' | run_shell)"
grep -q 'Running.*sleep 0.5 &' <<<"$out"

out="$(printf 'sleep 0.3 &\nfg %%1\necho after\nexit\n' | run_shell)"
grep -q 'after' <<<"$out"

printf 'PASS: basic execution, pipeline, redirection, variable expansion, background jobs, jobs, and fg\n'
