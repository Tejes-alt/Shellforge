# ShellForge

A small Unix-style shell written in C for **Ubuntu on WSL2 / Windows**. It is designed to be built natively inside Ubuntu so the executable always matches the user's Windows/WSL x86-64 environment.

## Features
- `fork`, `execvp`, `waitpid`
- pipes: `|`
- redirection: `<`, `>`, `>>`
- background jobs: `&`
- SIGCHLD handling and job table
- job-control built-ins: `jobs`, `fg`, `bg`
- terminal process groups and foreground control when running interactively in a real TTY
- built-ins: `cd`, `pwd`, `echo`, `export`, `unset`, `history`, `jobs`, `fg`, `bg`, `help`, `exit`
- GNU Readline history and line editing
- quoted strings and backslash escaping
- environment variable commands through normal `exec`/`export`

## Windows / WSL2 setup
Open **Ubuntu** (WSL2), then:

```bash
cd /mnt/d/2-1/os/shellforge
sudo apt update
sudo apt install build-essential libreadline-dev
make clean
make
./shellforge
```

Do **not** use a Windows `.exe`. ShellForge is a POSIX/Linux program and should be run inside Ubuntu/WSL2. The build creates the correct native Linux binary on your machine.

## Quick test

Inside ShellForge:

```text
echo hello
ls | wc -l
echo hello > test.txt
cat test.txt
sleep 5 &
jobs
fg %1
```

Use `Ctrl+Z` on a foreground command such as `sleep 30`, then `jobs`, `bg`, and `fg` to exercise interactive job control.

## Automated checks

```bash
make test
```

## Project layout

```text
include/      public headers
src/          C implementation
 tests/       automated smoke tests
docs/         project notes
makefile      WSL/Linux build
```

## Job-control demonstration

In an Ubuntu/WSL terminal:

```text
shellforge:~$ sleep 30
```

Press **Ctrl+Z**. The shell should report the job as stopped and return to the prompt.

```text
shellforge:~$ jobs
shellforge:~$ bg
shellforge:~$ jobs
shellforge:~$ fg
```

Press **Ctrl+C** while a foreground `sleep 30` is running to terminate the foreground job and return to ShellForge.
