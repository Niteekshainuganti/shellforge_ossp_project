#ifndef SHELLFORGE_JOB_MONITOR_H
#define SHELLFORGE_JOB_MONITOR_H

#include <sys/types.h>

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
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
void job_monitor_mark_stopped(pid_t pid);
void job_monitor_mark_running(pid_t pid);

void job_monitor_print(void);

pid_t job_monitor_get_pid(int job_id);
const char *job_monitor_get_command(int job_id);

void job_monitor_shutdown(void);

#endif
