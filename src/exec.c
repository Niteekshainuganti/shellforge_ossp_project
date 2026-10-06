#include "exec.h"
#include "signals.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

static int status_to_exit_code(int status) {
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return 1;
}

static int apply_redirections(const Command *cmd) {
    int fd;

    /* Standard input */
    if (cmd->input_file) {
        fd = open(cmd->input_file, O_RDONLY);

        if (fd < 0) {
            fprintf(stderr,
                    "shellforge: %s: %s\n",
                    cmd->input_file,
                    strerror(errno));
            return -1;
        }

        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("shellforge: dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    /* Standard output */
    if (cmd->output_file) {
        int flags = O_WRONLY | O_CREAT;

        if (cmd->append_output) {
            flags |= O_APPEND;
        } else {
            flags |= O_TRUNC;
        }

        fd = open(cmd->output_file, flags, 0644);

        if (fd < 0) {
            fprintf(stderr,
                    "shellforge: %s: %s\n",
                    cmd->output_file,
                    strerror(errno));
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("shellforge: dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    /* Standard error */
    if (cmd->error_file) {
        int flags = O_WRONLY | O_CREAT;

        if (cmd->append_error) {
            flags |= O_APPEND;
        } else {
            flags |= O_TRUNC;
        }

        fd = open(cmd->error_file, flags, 0644);

        if (fd < 0) {
            fprintf(stderr,
                    "shellforge: %s: %s\n",
                    cmd->error_file,
                    strerror(errno));
            return -1;
        }

        if (dup2(fd, STDERR_FILENO) < 0) {
            perror("shellforge: dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    return 0;
}

static void close_all_pipes(int *pipe_fds, size_t count) {
    if (!pipe_fds) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        close(pipe_fds[i]);
    }
}

static pid_t spawn_stage(const Command *cmd,
                         int in_fd,
                         int out_fd,
                         int *pipe_fds,
                         size_t pipe_count) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("shellforge: fork");
        return -1;
    }

    if (pid == 0) {
        child_reset_signal_handlers();

        /*
         * Pipeline connections are established first.
         * Explicit redirection then overrides them if requested.
         */
        if (in_fd != STDIN_FILENO) {
            if (dup2(in_fd, STDIN_FILENO) < 0) {
                perror("shellforge: dup2");
                _exit(126);
            }
        }

        if (out_fd != STDOUT_FILENO) {
            if (dup2(out_fd, STDOUT_FILENO) < 0) {
                perror("shellforge: dup2");
                _exit(126);
            }
        }

        close_all_pipes(pipe_fds, pipe_count);

        if (apply_redirections(cmd) != 0) {
            _exit(126);
        }

        execvp(cmd->argv[0], cmd->argv);

        fprintf(stderr,
                "shellforge: %s: %s\n",
                cmd->argv[0],
                strerror(errno));

        _exit(127);
    }

    return pid;
}

static int run_builtin_with_redirection(const Command *cmd,
                                        int *last_status,
                                        int *should_exit) {
    if (!cmd->input_file &&
        !cmd->output_file &&
        !cmd->error_file) {
        return 0;
    }

    /* Only handle redirection here for actual shell built-ins. */
    if (strcmp(cmd->argv[0], "cd") != 0 &&
        strcmp(cmd->argv[0], "exit") != 0 &&
        strcmp(cmd->argv[0], "pwd") != 0 &&
        strcmp(cmd->argv[0], "help") != 0 &&
        strcmp(cmd->argv[0], "memstat") != 0) {
        return 0;
    }

    int saved_stdin = -1;
    int saved_stdout = -1;
    int saved_stderr = -1;

    saved_stdin = dup(STDIN_FILENO);
    saved_stdout = dup(STDOUT_FILENO);
    saved_stderr = dup(STDERR_FILENO);

    if (saved_stdin < 0 ||
        saved_stdout < 0 ||
        saved_stderr < 0) {
        perror("shellforge: dup");
        return -1;
    }

    if (apply_redirections(cmd) != 0) {
        dup2(saved_stdin, STDIN_FILENO);
        dup2(saved_stdout, STDOUT_FILENO);
        dup2(saved_stderr, STDERR_FILENO);

        close(saved_stdin);
        close(saved_stdout);
        close(saved_stderr);

        *last_status = 126;
        return 1;
    }

    /*
     * Built-ins are handled by the existing shell logic.
     * We only temporarily redirect their standard streams here.
     */
    if (strcmp(cmd->argv[0], "cd") == 0) {
        int rc;

        if (cmd->argc < 2) {
            rc = chdir(getenv("HOME") ? getenv("HOME") : "/");
        } else {
            rc = chdir(cmd->argv[1]);
        }

        if (rc != 0) {
            perror("shellforge: cd");
            *last_status = 1;
        } else {
            *last_status = 0;
        }

        *should_exit = 0;
    } else if (strcmp(cmd->argv[0], "pwd") == 0) {
        char cwd[4096];

        if (getcwd(cwd, sizeof(cwd))) {
            printf("%s\n", cwd);
            *last_status = 0;
        } else {
            perror("shellforge: pwd");
            *last_status = 1;
        }

        *should_exit = 0;
    } else if (strcmp(cmd->argv[0], "help") == 0) {
        printf("Built-ins: cd exit pwd help memstat\n");
        *last_status = 0;
        *should_exit = 0;
    } else if (strcmp(cmd->argv[0], "memstat") == 0) {
        FILE *fp = fopen("/proc/self/status", "r");

        if (!fp) {
            perror("shellforge: memstat");
            *last_status = 1;
        } else {
            char line[256];

            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "VmSize:", 7) == 0 ||
                    strncmp(line, "VmRSS:", 6) == 0 ||
                    strncmp(line, "VmPeak:", 7) == 0) {
                    fputs(line, stdout);
                }
            }

            fclose(fp);
            *last_status = 0;
        }

        *should_exit = 0;
    } else if (strcmp(cmd->argv[0], "exit") == 0) {
        *last_status = 0;
        *should_exit = 1;
    }

    fflush(stdout);
    fflush(stderr);

    dup2(saved_stdin, STDIN_FILENO);
    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stderr, STDERR_FILENO);

    close(saved_stdin);
    close(saved_stdout);
    close(saved_stderr);

    return 1;
}

void run_pipeline(const Pipeline *pipeline,
                  int *last_status,
                  int *should_exit) {
    *last_status = 0;
    *should_exit = 0;

    size_t n = pipeline->commands.len;

    if (n == 0) {
        return;
    }

    Command *first_cmd = pipeline->commands.items[0];

    /*
     * Preserve existing built-in behavior for a single command.
     * Built-ins with redirection are handled here.
     */
    if (n == 1) {
        if (run_builtin_with_redirection(
                first_cmd,
                last_status,
                should_exit)) {
            return;
        }

        if (strcmp(first_cmd->argv[0], "cd") == 0) {
            if (first_cmd->argc < 2) {
                const char *home = getenv("HOME");

                if (home && chdir(home) != 0) {
                    perror("shellforge: cd");
                    *last_status = 1;
                } else {
                    *last_status = 0;
                }
            } else if (chdir(first_cmd->argv[1]) != 0) {
                perror("shellforge: cd");
                *last_status = 1;
            } else {
                *last_status = 0;
            }

            return;
        }

        if (strcmp(first_cmd->argv[0], "exit") == 0) {
            *should_exit = 1;
            *last_status = 0;
            return;
        }

        if (strcmp(first_cmd->argv[0], "pwd") == 0) {
            char cwd[4096];

            if (getcwd(cwd, sizeof(cwd))) {
                printf("%s\n", cwd);
                *last_status = 0;
            } else {
                perror("shellforge: pwd");
                *last_status = 1;
            }

            return;
        }

        if (strcmp(first_cmd->argv[0], "help") == 0) {
            printf("Built-ins: cd exit pwd help memstat\n");
            *last_status = 0;
            return;
        }

        if (strcmp(first_cmd->argv[0], "memstat") == 0) {
            FILE *fp = fopen("/proc/self/status", "r");

            if (!fp) {
                perror("shellforge: memstat");
                *last_status = 1;
                return;
            }

            char line[256];

            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "VmSize:", 7) == 0 ||
                    strncmp(line, "VmRSS:", 6) == 0 ||
                    strncmp(line, "VmPeak:", 7) == 0) {
                    fputs(line, stdout);
                }
            }

            fclose(fp);
            *last_status = 0;
            return;
        }
    }

    size_t pipe_count = n > 1 ? 2 * (n - 1) : 0;

    int *pipe_fds = NULL;

    if (pipe_count > 0) {
        pipe_fds = malloc(sizeof(int) * pipe_count);

        if (!pipe_fds) {
            perror("shellforge: malloc");
            *last_status = 1;
            return;
        }

        for (size_t i = 0; i < n - 1; i++) {
            int p[2];

            if (pipe(p) < 0) {
                perror("shellforge: pipe");
                free(pipe_fds);
                *last_status = 1;
                return;
            }

            pipe_fds[2 * i] = p[0];
            pipe_fds[2 * i + 1] = p[1];
        }
    }

    pid_t *pids = malloc(sizeof(pid_t) * n);

    if (!pids) {
        perror("shellforge: malloc");
        close_all_pipes(pipe_fds, pipe_count);
        free(pipe_fds);
        *last_status = 1;
        return;
    }

    sigchld_block();

    size_t spawned = 0;

    for (size_t i = 0; i < n; i++) {
        int in_fd = STDIN_FILENO;
        int out_fd = STDOUT_FILENO;

        if (i > 0) {
            in_fd = pipe_fds[2 * (i - 1)];
        }

        if (i < n - 1) {
            out_fd = pipe_fds[2 * i + 1];
        }

        pid_t pid = spawn_stage(
            pipeline->commands.items[i],
            in_fd,
            out_fd,
            pipe_fds,
            pipe_count
        );

        if (pid < 0) {
            break;
        }

        pids[spawned++] = pid;
    }

    close_all_pipes(pipe_fds, pipe_count);

    int final_status = 1;

    for (size_t i = 0; i < spawned; i++) {
        int status;

        if (waitpid(pids[i], &status, 0) < 0) {
            if (errno == EINTR) {
                i--;
                continue;
            }

            perror("shellforge: waitpid");
            continue;
        }

        if (i == spawned - 1) {
            final_status = status_to_exit_code(status);
        }
    }

    sigchld_unblock();

    free(pids);
    free(pipe_fds);

    *last_status = final_status;
}
