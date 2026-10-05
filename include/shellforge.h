#ifndef SHELLFORGE_H
#define SHELLFORGE_H

#include <sys/types.h>
#include <termios.h>
#include <stdbool.h>

#define SF_MAX_ARGS 128
#define SF_MAX_CMDS 32
#define SF_MAX_JOBS 64
#define SF_MAX_LINE 4096

typedef enum {
    REDIR_NONE,
    REDIR_IN,
    REDIR_OUT,
    REDIR_APPEND
} RedirType;

typedef struct {
    char *argv[SF_MAX_ARGS];
    int argc;
    char *infile;
    char *outfile;
    RedirType out_type;
} Command;

typedef struct {
    Command commands[SF_MAX_CMDS];
    int count;
    bool background;
    char *display;
} Pipeline;

typedef enum { JOB_RUNNING, JOB_STOPPED, JOB_DONE } JobState;

typedef struct {
    int id;
    pid_t pgid;
    pid_t pids[SF_MAX_CMDS];
    int npids;
    int live;
    int stopped;
    JobState state;
    char *command;
    int last_status;
} Job;

extern int shell_terminal;
extern pid_t shell_pgid;
extern struct termios shell_tmodes;
extern bool shell_interactive;

void jobs_init(void);
int jobs_add(pid_t pgid, const pid_t *pids, int npids, const char *command, bool stopped);
Job *jobs_get(int id);
Job *jobs_get_current(void);
Job *jobs_get_previous(void);
void jobs_update_status(pid_t pid, int status);
void jobs_mark_done(Job *job, int status);
void jobs_remove(Job *job);
void jobs_print(void);
void jobs_notify_done(void);

void job_control_init(void);
int put_job_foreground(Job *job, bool cont);
int put_job_background(Job *job, bool cont);
void sigchld_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);
void sigttou_handler(int sig);

int parse_line(const char *line, Pipeline *p);
void pipeline_free(Pipeline *p);
int execute_pipeline(Pipeline *p);
int is_builtin(const Command *cmd);
int execute_builtin(Command *cmd, bool parent_context);

char *sf_strdup(const char *s);
void trim_newline(char *s);

#endif
