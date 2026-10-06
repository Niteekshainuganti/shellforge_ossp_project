#include "parser.h"

#include <stdlib.h>
#include <string.h>

static void command_init(Command *cmd) {
    cmd->argv = NULL;
    cmd->argc = 0;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->error_file = NULL;
    cmd->append_output = 0;
    cmd->append_error = 0;
}

static void command_free(Command *cmd) {
    if (!cmd) {
        return;
    }

    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
    }

    free(cmd->argv);
    free(cmd->input_file);
    free(cmd->output_file);
    free(cmd->error_file);

    command_init(cmd);
}

static int argv_push(Command *cmd, const char *token) {
    char **new_argv = realloc(
        cmd->argv,
        sizeof(char *) * (size_t)(cmd->argc + 2)
    );

    if (!new_argv) {
        return -1;
    }

    cmd->argv = new_argv;

    cmd->argv[cmd->argc] = strdup(token);

    if (!cmd->argv[cmd->argc]) {
        return -1;
    }

    cmd->argc++;
    cmd->argv[cmd->argc] = NULL;

    return 0;
}

static int set_redirection(Command *cmd,
                           const char *op,
                           const char *filename) {
    if (strcmp(op, "<") == 0) {
        free(cmd->input_file);
        cmd->input_file = strdup(filename);
        return cmd->input_file ? 0 : -1;
    }

    if (strcmp(op, ">") == 0) {
        free(cmd->output_file);
        cmd->output_file = strdup(filename);
        cmd->append_output = 0;
        return cmd->output_file ? 0 : -1;
    }

    if (strcmp(op, ">>") == 0) {
        free(cmd->output_file);
        cmd->output_file = strdup(filename);
        cmd->append_output = 1;
        return cmd->output_file ? 0 : -1;
    }

    if (strcmp(op, "2>") == 0 || strcmp(op, "2>>") == 0) {
        free(cmd->error_file);
        cmd->error_file = strdup(filename);

        if (!cmd->error_file) {
            return -1;
        }

        cmd->append_error = (strcmp(op, "2>>") == 0);
        return 0;
    }

    return -1;
}

int parse_pipeline(const DVector *tokens,
                   Pipeline *pipeline_out,
                   const char **err_msg) {
    *err_msg = NULL;

    dv_init(&pipeline_out->commands);

    Command *current = malloc(sizeof(Command));

    if (!current) {
        *err_msg = "out of memory";
        return -1;
    }

    command_init(current);

    for (size_t i = 0; i < tokens->len; i++) {
        const char *token = tokens->items[i];

        /* Pipeline separator */
        if (strcmp(token, "|") == 0) {
            if (current->argc == 0) {
                command_free(current);
                free(current);
                *err_msg = "empty command in pipeline";
                return -1;
            }

            dv_push(&pipeline_out->commands, current);

            current = malloc(sizeof(Command));

            if (!current) {
                *err_msg = "out of memory";
                return -1;
            }

            command_init(current);
            continue;
        }

        /* Redirection operators */
        if (strcmp(token, "<") == 0 ||
            strcmp(token, ">") == 0 ||
            strcmp(token, ">>") == 0 ||
            strcmp(token, "2>") == 0 ||
            strcmp(token, "2>>") == 0) {

            if (i + 1 >= tokens->len) {
                command_free(current);
                free(current);
                *err_msg = "redirection requires a filename";
                return -1;
            }

            const char *filename = tokens->items[++i];

            if (strcmp(filename, "|") == 0 ||
                strcmp(filename, "<") == 0 ||
                strcmp(filename, ">") == 0 ||
                strcmp(filename, ">>") == 0 ||
                strcmp(filename, "2>") == 0 ||
                strcmp(filename, "2>>") == 0) {

                command_free(current);
                free(current);
                *err_msg = "redirection requires a filename";
                return -1;
            }

            if (set_redirection(current, token, filename) != 0) {
                command_free(current);
                free(current);
                *err_msg = "out of memory";
                return -1;
            }

            continue;
        }

        /* Normal command argument */
        if (argv_push(current, token) != 0) {
            command_free(current);
            free(current);
            *err_msg = "out of memory";
            return -1;
        }
    }

    if (current->argc == 0) {
        command_free(current);
        free(current);

        if (pipeline_out->commands.len == 0) {
            return 0;
        }

        *err_msg = "empty command in pipeline";
        return -1;
    }

    dv_push(&pipeline_out->commands, current);

    return 0;
}

void pipeline_free(Pipeline *p) {
    if (!p) {
        return;
    }

    for (size_t i = 0; i < p->commands.len; i++) {
        Command *cmd = p->commands.items[i];

        if (cmd) {
            command_free(cmd);
            free(cmd);
        }
    }

    dv_free(&p->commands);
}
