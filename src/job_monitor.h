#ifndef SHELLFORGE_JOB_MONITOR_H
#define SHELLFORGE_JOB_MONITOR_H

#include <sys/types.h>
#include <stddef.h>

typedef enum {
    JOB_RUNNING,
    JOB_FINISHED
} JobState;

typedef struct {
    int id;
    pid_t pid;
    char *command;
    JobState state;
    int exit_status;
} Job;

void job_monitor_init(void);
void job_monitor_add(pid_t pid, const char *command);
void job_monitor_mark_finished(pid_t pid, int exit_status);
void job_monitor_print(void);
void job_monitor_shutdown(void);

#endif
