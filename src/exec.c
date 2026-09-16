/* exec.c — Weeks 4 & 7: Process control (fork/exec/wait) + pipes */
#include "exec.h"
#include "builtins.h"
#include "signals.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static pid_t spawn_stage(const Command *cmd,
                         int in_fd,
                         int out_fd,
                         int *pipe_fds,
                         int n_pipe_fds) {
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "shellforge: fork: %s\n", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        child_reset_signal_handlers();

        if (in_fd != STDIN_FILENO) {
            if (dup2(in_fd, STDIN_FILENO) < 0) {
                _exit(126);
            }
        }

        if (out_fd != STDOUT_FILENO) {
            if (dup2(out_fd, STDOUT_FILENO) < 0) {
                _exit(126);
            }
        }

        for (int i = 0; i < n_pipe_fds; i++) {
            close(pipe_fds[i]);
        }

        execvp(cmd->argv[0], cmd->argv);

        if (errno == ENOENT) {
            fprintf(stderr,
                    "shellforge: %s: command not found\n",
                    cmd->argv[0]);
            _exit(127);
        } else {
            fprintf(stderr,
                    "shellforge: %s: %s\n",
                    cmd->argv[0],
                    strerror(errno));
            _exit(126);
        }
    }

    return pid;
}

static int status_to_exit_code(int status) {
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return 1;
}

void run_pipeline(const Pipeline *pipeline,
                  int *last_status,
                  int *should_exit) {
    *should_exit = 0;

    int n = (int)dv_len(&pipeline->commands);

    if (n == 0) {
        return;
    }

    if (n == 1) {
        Command *only =
            (Command *)dv_get(&pipeline->commands, 0);

        if (is_builtin(only)) {
            run_builtin(only, last_status, should_exit);
            return;
        }
    }

    int n_pipe_fds = 2 * (n - 1);
    int *pipe_fds = NULL;

    if (n_pipe_fds > 0) {
        pipe_fds = malloc((size_t)n_pipe_fds * sizeof(int));

        if (!pipe_fds) {
            fprintf(stderr,
                    "shellforge: out of memory (pipes)\n");
            *last_status = 1;
            return;
        }

        for (int i = 0; i < n - 1; i++) {
            if (pipe(&pipe_fds[i * 2]) != 0) {
                fprintf(stderr,
                        "shellforge: pipe: %s\n",
                        strerror(errno));

                for (int j = 0; j < i * 2; j++) {
                    close(pipe_fds[j]);
                }

                free(pipe_fds);
                *last_status = 1;
                return;
            }
        }
    }

    pid_t *pids = malloc((size_t)n * sizeof(pid_t));

    if (!pids) {
        fprintf(stderr,
                "shellforge: out of memory (pids)\n");

        for (int i = 0; i < n_pipe_fds; i++) {
            close(pipe_fds[i]);
        }

        free(pipe_fds);
        *last_status = 1;
        return;
    }

    sigchld_block();

    for (int i = 0; i < n; i++) {
        Command *cmd =
            (Command *)dv_get(&pipeline->commands, i);

        int in_fd = (i == 0)
            ? STDIN_FILENO
            : pipe_fds[(i - 1) * 2];

        int out_fd = (i == n - 1)
            ? STDOUT_FILENO
            : pipe_fds[i * 2 + 1];

        pids[i] = spawn_stage(
            cmd,
            in_fd,
            out_fd,
            pipe_fds,
            n_pipe_fds
        );
    }

    for (int i = 0; i < n_pipe_fds; i++) {
        close(pipe_fds[i]);
    }

    int final_status = 0;

    for (int i = 0; i < n; i++) {
        if (pids[i] < 0) {
            continue;
        }

        int status;
        pid_t r;

        do {
            r = waitpid(pids[i], &status, 0);
        } while (r < 0 && errno == EINTR);

        if (i == n - 1) {
            final_status =
                (r < 0) ? 1 : status_to_exit_code(status);
        }
    }

    sigchld_unblock();

    *last_status = final_status;

    free(pids);
    free(pipe_fds);
}

