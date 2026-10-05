# ShellForge architecture

```text
Readline
   |
   v
Parser / lexer-like scanner
   |
   v
Pipeline representation
   |
   +--> Built-in in parent (cd, export, jobs, fg, bg, ...)
   |
   +--> fork() -> process group -> pipe()/dup2()/redirection -> execvp()
                                |
                                v
                         Job table + SIGCHLD
                                |
                         jobs / fg / bg
```

Interactive job control uses POSIX process groups and the controlling terminal. The shell ignores terminal-generated job-control signals for itself, places each launched pipeline in a separate process group, transfers the terminal to foreground jobs, and takes it back when they stop or finish.
