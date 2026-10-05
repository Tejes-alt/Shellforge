#define _POSIX_C_SOURCE 200809L
#include "shellforge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

static Job jobs[SF_MAX_JOBS];
static int next_id = 1;

static void clear_job(Job *j) {
    if (!j) return;
    free(j->command);
    memset(j, 0, sizeof(*j));
}

void jobs_init(void) {
    memset(jobs, 0, sizeof(jobs));
    next_id = 1;
}

int jobs_add(pid_t pgid, const pid_t *pids, int npids, const char *command, bool stopped) {
    for (int i = 0; i < SF_MAX_JOBS; i++) {
        if (jobs[i].id != 0) continue;
        Job *j = &jobs[i];
        j->id = next_id++;
        if (next_id > 9999) next_id = 1;
        j->pgid = pgid;
        j->npids = npids;
        j->live = npids;
        j->stopped = stopped ? npids : 0;
        for (int k = 0; k < npids; k++) j->pids[k] = pids[k];
        j->state = stopped ? JOB_STOPPED : JOB_RUNNING;
        j->command = sf_strdup(command ? command : "");
        return j->id;
    }
    fprintf(stderr, "shellforge: job table full\n");
    return -1;
}

Job *jobs_get(int id) {
    for (int i = 0; i < SF_MAX_JOBS; i++)
        if (jobs[i].id == id) return &jobs[i];
    return NULL;
}

Job *jobs_get_current(void) {
    Job *best = NULL;
    for (int i = 0; i < SF_MAX_JOBS; i++) {
        if (jobs[i].id == 0 || jobs[i].state == JOB_DONE) continue;
        if (!best || jobs[i].id > best->id) best = &jobs[i];
    }
    return best;
}

Job *jobs_get_previous(void) {
    Job *current = jobs_get_current();
    Job *best = NULL;
    for (int i = 0; i < SF_MAX_JOBS; i++) {
        if (jobs[i].id == 0 || jobs[i].state == JOB_DONE || &jobs[i] == current) continue;
        if (!best || jobs[i].id > best->id) best = &jobs[i];
    }
    return best;
}

void jobs_mark_done(Job *j, int status) {
    if (!j) return;
    j->live = 0;
    j->stopped = 0;
    j->last_status = status;
    j->state = JOB_DONE;
}

void jobs_update_status(pid_t pid, int status) {
    for (int i = 0; i < SF_MAX_JOBS; i++) {
        Job *j = &jobs[i];
        if (j->id == 0) continue;
        for (int k = 0; k < j->npids; k++) {
            if (j->pids[k] != pid) continue;
            if (WIFSTOPPED(status)) {
                j->stopped++;
                if (j->stopped >= j->live) j->state = JOB_STOPPED;
            } else if (WIFCONTINUED(status)) {
                if (j->stopped > 0) j->stopped--;
                j->state = JOB_RUNNING;
            } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                if (j->live > 0) j->live--;
                j->last_status = status;
                if (j->live == 0) jobs_mark_done(j, status);
            }
            return;
        }
    }
}

void jobs_remove(Job *j) { clear_job(j); }

static const char *state_name(const Job *j) {
    if (j->state == JOB_STOPPED) return "Stopped";
    if (j->state == JOB_DONE) return "Done";
    return "Running";
}

void jobs_print(void) {
    for (int i = 0; i < SF_MAX_JOBS; i++) {
        if (jobs[i].id == 0) continue;
        printf("[%d] %-8s %s\n", jobs[i].id, state_name(&jobs[i]), jobs[i].command);
    }
    fflush(stdout);
}

void jobs_notify_done(void) {
    for (int i = 0; i < SF_MAX_JOBS; i++) {
        if (jobs[i].id == 0 || jobs[i].state != JOB_DONE) continue;
        printf("[%d] Done\t%s\n", jobs[i].id, jobs[i].command);
        clear_job(&jobs[i]);
    }
    fflush(stdout);
}
