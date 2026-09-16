/* parser.c — Week 3: Pipeline AST builder */
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Command *command_new(void) {
    Command *cmd = calloc(1, sizeof(Command));

    if (!cmd) {
        fprintf(stderr, "shellforge: out of memory (command)\n");
        exit(1);
    }

    cmd->argv = NULL;
    cmd->argc = 0;

    return cmd;
}

static void command_add_arg(Command *cmd, const char *word) {
    char **new_argv = realloc(
        cmd->argv,
        (size_t)(cmd->argc + 2) * sizeof(char *)
    );

    if (!new_argv) {
        fprintf(stderr, "shellforge: out of memory (argv)\n");
        exit(1);
    }

    cmd->argv = new_argv;
    cmd->argv[cmd->argc] = strdup(word);
    cmd->argc++;
    cmd->argv[cmd->argc] = NULL;
}

static void command_free(Command *cmd) {
    if (!cmd) return;

    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->argv[i]);
    }

    free(cmd->argv);
    free(cmd);
}

int parse_pipeline(const DVector *tokens,
                   Pipeline *pipeline_out,
                   const char **err_msg) {
    dv_init(&pipeline_out->commands);

    size_t n = dv_len(tokens);

    if (n == 0) {
        return 0;
    }

    Command *cur = command_new();
    int cur_has_args = 0;
    int prev_was_pipe = 1;

    for (size_t i = 0; i < n; i++) {
        const char *tok = (const char *)dv_get(tokens, i);

        if (strcmp(tok, "|") == 0) {
            if (prev_was_pipe) {
                command_free(cur);
                pipeline_free(pipeline_out);
                *err_msg = "syntax error: unexpected '|'";
                return -1;
            }

            dv_push(&pipeline_out->commands, cur);

            cur = command_new();
            cur_has_args = 0;
            prev_was_pipe = 1;
            continue;
        }

        command_add_arg(cur, tok);
        cur_has_args = 1;
        prev_was_pipe = 0;
    }

    if (prev_was_pipe) {
        command_free(cur);
        pipeline_free(pipeline_out);
        *err_msg = "syntax error: expected command after '|'";
        return -1;
    }

    if (cur_has_args) {
        dv_push(&pipeline_out->commands, cur);
    } else {
        command_free(cur);
    }

    return 0;
}

void pipeline_free(Pipeline *p) {
    for (size_t i = 0; i < dv_len(&p->commands); i++) {
        command_free((Command *)dv_get(&p->commands, i));
    }

    dv_free(&p->commands);
}
