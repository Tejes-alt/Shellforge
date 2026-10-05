# Windows / WSL2 notes

ShellForge is a Linux/POSIX program. On Windows, the intended environment is **Ubuntu under WSL2**.

For the project path shown in the original setup:

```bash
cd /mnt/d/2-1/os/shellforge
```

Install dependencies once:

```bash
sudo apt update
sudo apt install build-essential libreadline-dev
```

Build natively in WSL:

```bash
make clean
make
```

Run:

```bash
./shellforge
```

Do not try to run the Linux binary directly from PowerShell or `cmd.exe`. Open Ubuntu/WSL and run it there. This avoids architecture and runtime-library mismatches from copying prebuilt binaries between machines.
