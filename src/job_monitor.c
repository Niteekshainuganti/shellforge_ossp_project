#include "job_monitor.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INITIAL_CAPACITY 8

static Job *jobs = NULL;
static size_t job_count = 0;
static size_t job_capacity = 0;

static pthread_mutex_t jobs_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t monitor_thread;
static int monitor_running = 0;

static void *monitor_worker(void *arg)
{
    (void)arg;

    JobState *last_states = NULL;
    size_t last_count = 0;

    while (monitor_running) {
        pthread_mutex_lock(&jobs_mutex);

        if (job_count > last_count) {
            JobState *new_states =
                realloc(last_states, job_count * sizeof(JobState));

            if (new_states != NULL) {
                last_states = new_states;

                for (size_t i = last_count; i < job_count; i++) {
                    last_states[i] = (JobState)-1;

                    printf("[monitor] Job %d | PID %d | RUNNING\n",
                           jobs[i].id,
                           (int)jobs[i].pid);
                }

                last_count = job_count;
            }
        }

        for (size_t i = 0; i < job_count; i++) {
            if (last_states != NULL &&
                jobs[i].state != last_states[i]) {

                if (last_states[i] != (JobState)-1) {
                    const char *state =
                        jobs[i].state == JOB_RUNNING
                            ? "RUNNING"
                            : "FINISHED";

                    printf("[monitor] Job %d | PID %d | %s\n",
                           jobs[i].id,
                           (int)jobs[i].pid,
                           state);
                }

                last_states[i] = jobs[i].state;
            }
        }

        pthread_mutex_unlock(&jobs_mutex);

        sleep(1);
    }

    free(last_states);
    return NULL;
}

void job_monitor_init(void)
{
    pthread_mutex_lock(&jobs_mutex);

    jobs = calloc(INITIAL_CAPACITY, sizeof(Job));
    job_count = 0;
    job_capacity = jobs ? INITIAL_CAPACITY : 0;
    monitor_running = jobs != NULL;

    pthread_mutex_unlock(&jobs_mutex);

    if (monitor_running) {
        if (pthread_create(&monitor_thread, NULL,
                           monitor_worker, NULL) != 0) {
            pthread_mutex_lock(&jobs_mutex);
            monitor_running = 0;
            free(jobs);
            jobs = NULL;
            job_capacity = 0;
            pthread_mutex_unlock(&jobs_mutex);

            perror("shellforge: pthread_create");
        }
    }
}

void job_monitor_add(pid_t pid, const char *command)
{
    pthread_mutex_lock(&jobs_mutex);

    if (job_count == job_capacity) {
        size_t new_capacity = job_capacity * 2;

        if (new_capacity == 0)
            new_capacity = INITIAL_CAPACITY;

        Job *new_jobs = realloc(jobs, new_capacity * sizeof(Job));

        if (!new_jobs) {
            pthread_mutex_unlock(&jobs_mutex);
            return;
        }

        jobs = new_jobs;
        job_capacity = new_capacity;
    }

    Job *job = &jobs[job_count];

    job->id = (int)job_count + 1;
    job->pid = pid;
    job->command = strdup(command ? command : "");
    job->state = JOB_RUNNING;
    job->exit_status = -1;

    job_count++;

    pthread_mutex_unlock(&jobs_mutex);
}

void job_monitor_mark_finished(pid_t pid, int exit_status)
{
    pthread_mutex_lock(&jobs_mutex);

    for (size_t i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            jobs[i].state = JOB_FINISHED;
            jobs[i].exit_status = exit_status;
            break;
        }
    }

    pthread_mutex_unlock(&jobs_mutex);
}

void job_monitor_print(void)
{
    pthread_mutex_lock(&jobs_mutex);

    printf("Job Monitor:\n");

    if (job_count == 0) {
        printf("  No jobs.\n");
    }

    for (size_t i = 0; i < job_count; i++) {
        const char *state =
            (jobs[i].state == JOB_RUNNING) ? "RUNNING" : "FINISHED";

        printf("  [%d] PID %d  %-8s  %s\n",
               jobs[i].id,
               (int)jobs[i].pid,
               state,
               jobs[i].command ? jobs[i].command : "");
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

    for (size_t i = 0; i < job_count; i++) {
        free(jobs[i].command);
    }

    free(jobs);
    jobs = NULL;
    job_count = 0;
    job_capacity = 0;

    pthread_mutex_unlock(&jobs_mutex);
}
