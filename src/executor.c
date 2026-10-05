#define _POSIX_C_SOURCE 200809L

#include "shellforge.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

static int redirect_child(Command *c)
{
    if (c->infile) {
        int fd = open(c->infile, O_RDONLY);

        if (fd < 0) {
            perror(c->infile);
            return -1;
        }

        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    if (c->outfile) {
        int flags = O_WRONLY | O_CREAT;

        if (c->out_type == REDIR_APPEND)
            flags |= O_APPEND;
        else
            flags |= O_TRUNC;

        int fd = open(c->outfile, flags, 0666);

        if (fd < 0) {
            perror(c->outfile);
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    return 0;
}

int execute_pipeline(Pipeline *p)
{
    if (!p || p->count == 0)
        return 0;

    /*
     * A single foreground builtin executes directly
     * inside the shell process.
     */
    if (p->count == 1 &&
        !p->background &&
        is_builtin(&p->commands[0])) {

        Command *c = &p->commands[0];

        int saved_in = -1;
        int saved_out = -1;

        if (c->infile || c->outfile) {

            saved_in = dup(STDIN_FILENO);
            saved_out = dup(STDOUT_FILENO);

            if (saved_in < 0 ||
                saved_out < 0 ||
                redirect_child(c) < 0) {

                if (saved_in >= 0) {
                    dup2(saved_in, STDIN_FILENO);
                    close(saved_in);
                }

                if (saved_out >= 0) {
                    dup2(saved_out, STDOUT_FILENO);
                    close(saved_out);
                }

                return 1;
            }
        }

        int result = execute_builtin(c, true);

        if (saved_in >= 0) {
            dup2(saved_in, STDIN_FILENO);
            close(saved_in);
        }

        if (saved_out >= 0) {
            dup2(saved_out, STDOUT_FILENO);
            close(saved_out);
        }

        return result;
    }

    /*
     * Prevent SIGCHLD from reaping a child before
     * the job has been added to the job table.
     */
    sigset_t block;
    sigset_t oldmask;

    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &block, &oldmask) < 0) {
        perror("sigprocmask");
        return 1;
    }

    int pipes[SF_MAX_CMDS - 1][2];

    for (int i = 0; i < p->count - 1; i++) {

        if (pipe(pipes[i]) < 0) {
            perror("pipe");

            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            sigprocmask(SIG_SETMASK, &oldmask, NULL);

            return 1;
        }
    }

    pid_t pids[SF_MAX_CMDS];

    pid_t pgid = 0;

    int npids = 0;

    for (int i = 0; i < p->count; i++) {

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");

            for (int k = 0; k < p->count - 1; k++) {
                close(pipes[k][0]);
                close(pipes[k][1]);
            }

            sigprocmask(SIG_SETMASK, &oldmask, NULL);

            return 1;
        }

        if (pid == 0) {

            /*
             * Restore normal signal handling in children.
             */
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);

            pid_t self = getpid();

            if (pgid == 0)
                pgid = self;

            setpgid(0, pgid);

            /*
             * Connect pipe input.
             */
            if (i > 0) {
                if (dup2(pipes[i - 1][0],
                         STDIN_FILENO) < 0) {
                    _exit(126);
                }
            }

            /*
             * Connect pipe output.
             */
            if (i < p->count - 1) {
                if (dup2(pipes[i][1],
                         STDOUT_FILENO) < 0) {
                    _exit(126);
                }
            }

            /*
             * Close all pipe descriptors.
             */
            for (int k = 0; k < p->count - 1; k++) {
                close(pipes[k][0]);
                close(pipes[k][1]);
            }

            /*
             * Apply file redirection.
             */
            if (redirect_child(&p->commands[i]) < 0)
                _exit(126);

            /*
             * Builtins inside pipelines/background jobs
             * execute inside the child.
             */
            if (is_builtin(&p->commands[i])) {

                int result =
                    execute_builtin(&p->commands[i],
                                    false);

                fflush(NULL);

                _exit(result);
            }

            /*
             * External command.
             */
            execvp(p->commands[i].argv[0],
                   p->commands[i].argv);

            fprintf(stderr,
                    "shellforge: %s: command not found\n",
                    p->commands[i].argv[0]);

            _exit(127);
        }

        /*
         * Parent.
         */
        if (pgid == 0)
            pgid = pid;

        setpgid(pid, pgid);

        pids[npids++] = pid;
    }

    /*
     * Parent no longer needs pipe descriptors.
     */
    for (int i = 0; i < p->count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    /*
     * Register the complete process group as one job
     * before allowing SIGCHLD to run.
     */
    int job_id =
        jobs_add(pgid,
                 pids,
                 npids,
                 p->display,
                 false);

    if (job_id < 0) {

        kill(-pgid, SIGTERM);

        sigprocmask(SIG_SETMASK,
                    &oldmask,
                    NULL);

        return 1;
    }

    Job *job = jobs_get(job_id);

    if (!job) {

        sigprocmask(SIG_SETMASK,
                    &oldmask,
                    NULL);

        return 1;
    }

    /*
     * Foreground command.
     *
     * SIGCHLD is still blocked here, so
     * put_job_foreground() can safely wait.
     */
    if (!p->background) {

        int result =
            put_job_foreground(job, false);

        return result;
    }

    /*
     * Background command.
     */
    printf("[%d] %d\n",
           job_id,
           pgid);

    fflush(stdout);

    /*
     * Now allow SIGCHLD to notify us when the
     * background job changes state.
     */
    if (sigprocmask(SIG_SETMASK,
                    &oldmask,
                    NULL) < 0) {
        perror("sigprocmask");
    }

    return 0;
}
