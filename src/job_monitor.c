#include "job_monitor.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Job *jobs = NULL;
static size_t job_count = 0;
static size_t job_capacity = 0;
static int next_job_id = 1;

static pthread_mutex_t jobs_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t monitor_thread;
static int monitor_running = 0;

static Job *find_job_by_pid(pid_t pid)
{
    for (size_t i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid)
            return &jobs[i];
    }

    return NULL;
}

static Job *find_job_by_id(int id)
{
    for (size_t i = 0; i < job_count; i++) {
        if (jobs[i].id == id)
            return &jobs[i];
    }

    return NULL;
}

static void *monitor_loop(void *arg)
{
    (void)arg;

    JobState *last_states = NULL;
    size_t last_capacity = 0;

    while (monitor_running) {
        pthread_mutex_lock(&jobs_mutex);

        if (job_count > last_capacity) {
            JobState *tmp = realloc(
                last_states,
                sizeof(JobState) * job_count
            );

            if (tmp != NULL) {
                last_states = tmp;

                for (size_t i = last_capacity; i < job_count; i++)
                    last_states[i] = JOB_FINISHED;

                last_capacity = job_count;
            }
        }

        for (size_t i = 0; i < job_count; i++) {
            if (i >= last_capacity)
                continue;

            if (jobs[i].state != last_states[i]) {
                const char *state =
                    jobs[i].state == JOB_RUNNING ? "RUNNING" :
                    jobs[i].state == JOB_STOPPED ? "STOPPED" :
                    "FINISHED";

                printf("[monitor] Job %d | PID %d | %s\n",
                       jobs[i].id,
                       (int)jobs[i].pid,
                       state);
                fflush(stdout);

                last_states[i] = jobs[i].state;
            }
        }

        pthread_mutex_unlock(&jobs_mutex);

        usleep(100000);
    }

    free(last_states);
    return NULL;
}

void job_monitor_init(void)
{
    pthread_mutex_lock(&jobs_mutex);

    monitor_running = 1;

    pthread_mutex_unlock(&jobs_mutex);

    pthread_create(&monitor_thread, NULL, monitor_loop, NULL);
}

void job_monitor_add(pid_t pid, const char *command)
{
    pthread_mutex_lock(&jobs_mutex);

    if (job_count == job_capacity) {
        size_t new_capacity =
            job_capacity == 0 ? 8 : job_capacity * 2;

        Job *tmp = realloc(
            jobs,
            sizeof(Job) * new_capacity
        );

        if (tmp == NULL) {
            pthread_mutex_unlock(&jobs_mutex);
            return;
        }

        jobs = tmp;
        job_capacity = new_capacity;
    }

    jobs[job_count].id = next_job_id++;
    jobs[job_count].pid = pid;
    jobs[job_count].command = strdup(command ? command : "");
    jobs[job_count].state = JOB_RUNNING;
    jobs[job_count].exit_status = 0;

    job_count++;

    pthread_mutex_unlock(&jobs_mutex);
}

void job_monitor_mark_finished(pid_t pid, int exit_status)
{
    pthread_mutex_lock(&jobs_mutex);

    Job *job = find_job_by_pid(pid);

    if (job != NULL) {
        job->state = JOB_FINISHED;
        job->exit_status = exit_status;
    }

    pthread_mutex_unlock(&jobs_mutex);
}

void job_monitor_mark_stopped(pid_t pid)
{
    pthread_mutex_lock(&jobs_mutex);

    Job *job = find_job_by_pid(pid);

    if (job != NULL)
        job->state = JOB_STOPPED;

    pthread_mutex_unlock(&jobs_mutex);
}

void job_monitor_mark_running(pid_t pid)
{
    pthread_mutex_lock(&jobs_mutex);

    Job *job = find_job_by_pid(pid);

    if (job != NULL)
        job->state = JOB_RUNNING;

    pthread_mutex_unlock(&jobs_mutex);
}

pid_t job_monitor_get_pid(int job_id)
{
    pthread_mutex_lock(&jobs_mutex);

    Job *job = find_job_by_id(job_id);
    pid_t pid = job ? job->pid : -1;

    pthread_mutex_unlock(&jobs_mutex);

    return pid;
}

const char *job_monitor_get_command(int job_id)
{
    pthread_mutex_lock(&jobs_mutex);

    Job *job = find_job_by_id(job_id);
    const char *command = job ? job->command : NULL;

    pthread_mutex_unlock(&jobs_mutex);

    return command;
}

void job_monitor_print(void)
{
    pthread_mutex_lock(&jobs_mutex);

    printf("Job Monitor:\n");

    if (job_count == 0) {
        printf("  No jobs.\n");
    } else {
        for (size_t i = 0; i < job_count; i++) {
            const char *state =
                jobs[i].state == JOB_RUNNING ? "RUNNING" :
                jobs[i].state == JOB_STOPPED ? "STOPPED" :
                "FINISHED";

            printf("  [%d] PID %d  %-8s  %s\n",
                   jobs[i].id,
                   (int)jobs[i].pid,
                   state,
                   jobs[i].command ? jobs[i].command : "");
        }
    }

    pthread_mutex_unlock(&jobs_mutex);
}

void job_monitor_shutdown(void)
{
    if (monitor_running) {
        monitor_running = 0;
        pthread_join(monitor_thread, NULL);
    }

    pthread_mutex_lock(&jobs_mutex);

    for (size_t i = 0; i < job_count; i++)
        free(jobs[i].command);

    free(jobs);

    jobs = NULL;
    job_count = 0;
    job_capacity = 0;

    pthread_mutex_unlock(&jobs_mutex);
}
