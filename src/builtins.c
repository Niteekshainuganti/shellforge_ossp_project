/* builtins.c — Week 5: PATH + built-ins, exit codes implementation */
#include "builtins.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

static const char *BUILTIN_NAMES[] = {
    "cd",
    "exit",
    "pwd",
    "help",
    NULL
};

int is_builtin(const Command *cmd) {
    if (cmd->argc == 0) return 0;

    for (int i = 0; BUILTIN_NAMES[i] != NULL; i++) {
        if (strcmp(cmd->argv[0], BUILTIN_NAMES[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

static void builtin_cd(const Command *cmd, int *last_status) {
    const char *target = NULL;

    if (cmd->argc < 2) {
        target = getenv("HOME");

        if (!target) {
            fprintf(stderr, "shellforge: cd: HOME not set\n");
            *last_status = 1;
            return;
        }
    } else {
        target = cmd->argv[1];
    }

    if (chdir(target) != 0) {
        fprintf(stderr, "shellforge: cd: %s: %s\n",
                target, strerror(errno));
        *last_status = 1;
        return;
    }

    *last_status = 0;
}

static void builtin_exit(const Command *cmd,
                         int *last_status,
                         int *should_exit) {
    int code = *last_status;

    if (cmd->argc >= 2) {
        char *endptr = NULL;

        long parsed = strtol(cmd->argv[1], &endptr, 10);

        if (endptr == cmd->argv[1] || *endptr != '\0') {
            fprintf(stderr,
                    "shellforge: exit: %s: numeric argument required\n",
                    cmd->argv[1]);
            code = 2;
        } else {
            code = (int)(parsed & 0xFF);
        }
    }

    *last_status = code;
    *should_exit = 1;
}

static void builtin_pwd(int *last_status) {
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "shellforge: pwd: %s\n", strerror(errno));
        *last_status = 1;
        return;
    }

    printf("%s\n", cwd);
    *last_status = 0;
}

static void builtin_help(int *last_status) {
    printf(
        "ShellForge — built-in commands:\n"
        "  cd [dir]     change working directory (default: $HOME)\n"
        "  pwd          print working directory\n"
        "  exit [code]  exit the shell\n"
        "  help         show this message\n"
        "Anything else is looked up on PATH and run as an external program.\n"
        "Pipelines of any length are supported.\n"
    );

    *last_status = 0;
}

void run_builtin(const Command *cmd,
                 int *last_status,
                 int *should_exit) {
    *should_exit = 0;

    if (strcmp(cmd->argv[0], "cd") == 0) {
        builtin_cd(cmd, last_status);
    } else if (strcmp(cmd->argv[0], "exit") == 0) {
        builtin_exit(cmd, last_status, should_exit);
    } else if (strcmp(cmd->argv[0], "pwd") == 0) {
        builtin_pwd(last_status);
    } else if (strcmp(cmd->argv[0], "help") == 0) {
        builtin_help(last_status);
    } else {
        fprintf(stderr,
                "shellforge: %s: not actually a builtin\n",
                cmd->argv[0]);
        *last_status = 127;
    }
}
