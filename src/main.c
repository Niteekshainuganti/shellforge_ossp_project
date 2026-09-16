/* main.c — Week 1: The Machine Beneath the Prompt */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dstring.h"
#include "dvector.h"
#include "tokenizer.h"
#include "parser.h"
#include "exec.h"
#include "signals.h"

static int read_line(DString *out) {
    ds_clear(out);

    int c;
    int got_any = 0;

    while ((c = getchar()) != EOF) {
        got_any = 1;

        if (c == '\n') {
            break;
        }

        ds_append_char(out, (char)c);
    }

    if (!got_any && c == EOF) {
        return -1;
    }

    return 0;
}

static void print_prompt(int last_status) {
    char cwd[PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        strcpy(cwd, "?");
    }

    if (last_status != 0) {
        printf("shellforge [%d] %s $ ", last_status, cwd);
    } else {
        printf("shellforge %s $ ", cwd);
    }

    fflush(stdout);
}

int main(void) {
    shell_install_signal_handlers();

    DString line;
    ds_init(&line);

    int last_status = 0;

    while (1) {
        print_prompt(last_status);

        if (read_line(&line) < 0) {
            printf("\n");
            break;
        }

        DVector tokens;
        const char *err_msg = NULL;

        if (tokenize(ds_cstr(&line), &tokens, &err_msg) != 0) {
            fprintf(stderr, "shellforge: %s\n", err_msg);
            continue;
        }

        Pipeline pipeline;

        if (parse_pipeline(&tokens, &pipeline, &err_msg) != 0) {
            fprintf(stderr, "shellforge: %s\n", err_msg);
            dv_free_deep(&tokens);
            continue;
        }

        int should_exit = 0;

        run_pipeline(
            &pipeline,
            &last_status,
            &should_exit
        );

        pipeline_free(&pipeline);
        dv_free_deep(&tokens);

        if (should_exit) {
            break;
        }
    }

    ds_free(&line);

    return last_status;
}
