#define _POSIX_C_SOURCE 200809L

#include "shellforge.h"

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int shell_terminal;
pid_t shell_pgid;
struct termios shell_tmodes;
bool shell_interactive;

void sigchld_handler(int sig)
{
    (void)sig;

    int saved_errno = errno;
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status,
                          WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        jobs_update_status(pid, status);
    }

    errno = saved_errno;
}

void sigint_handler(int sig)
{
    (void)sig;
}

void sigtstp_handler(int sig)
{
    (void)sig;
}

void sigttou_handler(int sig)
{
    (void)sig;
}

void job_control_init(void)
{
    shell_terminal = STDIN_FILENO;
    shell_interactive = isatty(shell_terminal);

    signal(SIGCHLD, sigchld_handler);

    if (!shell_interactive)
        return;

    while (tcgetpgrp(shell_terminal) !=
           (shell_pgid = getpgrp())) {
        kill(-shell_pgid, SIGTTIN);
    }

    shell_pgid = getpid();

    if (setpgid(shell_pgid, shell_pgid) < 0 &&
        errno != EACCES) {
        if (errno == EPERM) {
            shell_interactive = false;
            return;
        }

        perror("setpgid");
        exit(EXIT_FAILURE);
    }

    if (tcsetpgrp(shell_terminal, shell_pgid) < 0) {
        if (errno == EPERM) {
            shell_interactive = false;
            return;
        }

        perror("tcsetpgrp");
        exit(EXIT_FAILURE);
    }

    tcgetattr(shell_terminal, &shell_tmodes);

    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}

static void wait_for_job(Job *job)
{
    int status;
    pid_t pid;

    while (job->live > 0 &&
           job->state != JOB_STOPPED) {

        pid = waitpid(-job->pgid,
                      &status,
                      WUNTRACED);

        if (pid < 0) {
            if (errno == EINTR)
                continue;

            if (errno == ECHILD)
                break;

            perror("waitpid");
            break;
        }

        jobs_update_status(pid, status);
    }
}

int put_job_foreground(Job *job, bool cont)
{
    if (!job)
        return 1;

    /*
     * IMPORTANT:
     * Block SIGCHLD so the SIGCHLD handler cannot reap
     * the foreground process before waitpid() does.
     */
    sigset_t block;
    sigset_t oldmask;

    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &block, &oldmask) < 0) {
        perror("sigprocmask");
        return 1;
    }

    int job_id = job->id;

    if (shell_interactive) {
        if (tcsetpgrp(shell_terminal, job->pgid) < 0) {
            perror("tcsetpgrp");
        }
    }

    if (cont) {
        if (kill(-job->pgid, SIGCONT) < 0 &&
            errno != ESRCH) {
            perror("fg");
        }

        job->state = JOB_RUNNING;
        job->stopped = 0;
    }

    wait_for_job(job);

    int status = job->last_status;
    bool stopped = (job->state == JOB_STOPPED);

    char *command = NULL;

    if (stopped) {
        command = sf_strdup(job->command);
    }

    /*
     * Give terminal control back to ShellForge.
     */
    if (shell_interactive) {
        if (tcsetpgrp(shell_terminal, shell_pgid) < 0) {
            perror("tcsetpgrp");
        }

        tcgetattr(shell_terminal, &shell_tmodes);
    }

    if (stopped) {
        fprintf(stderr,
                "\n[%d]+ Stopped\t%s\n",
                job_id,
                command ? command : "");

        free(command);
    } else {
        jobs_remove(job);
    }

    /*
     * Re-enable SIGCHLD.
     * If a child changed state while it was blocked,
     * the signal will be delivered now.
     */
    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        perror("sigprocmask");
    }

    return status;
}

int put_job_background(Job *job, bool cont)
{
    if (!job)
        return 1;

    if (cont) {
        if (kill(-job->pgid, SIGCONT) < 0 &&
            errno != ESRCH) {
            perror("bg");
            return 1;
        }

        job->state = JOB_RUNNING;
        job->stopped = 0;
    }

    printf("[%d] %d\n",
           job->id,
           job->pgid);

    fflush(stdout);

    return 0;
}
