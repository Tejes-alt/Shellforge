# ShellForge testing checklist

1. `pwd`, `ls`, `date` — external commands and built-ins.
2. `echo hello | cat` — pipeline.
3. `echo hello > test.txt`, `cat < test.txt`, `echo world >> test.txt` — redirection.
4. `sleep 5 &` — background launch.
5. `jobs` — job table.
6. `sleep 30`, press Ctrl+Z, then `jobs` — stopped job.
7. `bg` — continue stopped job in background.
8. `fg` — bring job to foreground.
9. `Ctrl+C` while `sleep 30` is foreground — interrupt the child, not the shell.
10. `history` and arrow keys — Readline history.

For WSL2, all of these should be performed in an Ubuntu terminal/TTY, not from a Windows `cmd.exe` or PowerShell prompt.
