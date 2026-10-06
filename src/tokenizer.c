#include "tokenizer.h"
#include "dstring.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static void push_token(DVector *out, DString *cur) {
    if (cur->len == 0) return;

    char *tok = strdup(ds_cstr(cur));
    if (tok != NULL)
        dv_push(out, tok);

    ds_clear(cur);
}

static void push_operator(DVector *out, const char *op) {
    char *tok = strdup(op);
    if (tok != NULL)
        dv_push(out, tok);
}

int tokenize(const char *line, DVector *out_tokens, const char **err_msg) {
    *err_msg = NULL;

    DString cur;
    ds_init(&cur);
    dv_init(out_tokens);

    size_t i = 0;
    size_t n = strlen(line);
    int had_content = 0;

    while (i < n) {
        char c = line[i];

        if (c == '\'' || c == '"') {
            char quote = c;
            i++;
            had_content = 1;

            while (i < n && line[i] != quote) {
                ds_append_char(&cur, line[i]);
                i++;
            }

            if (i >= n) {
                ds_free(&cur);
                *err_msg = "unterminated quote";
                return -1;
            }

            i++;
            continue;
        }

        if (c == '\\' && i + 1 < n) {
            ds_append_char(&cur, line[i + 1]);
            had_content = 1;
            i += 2;
            continue;
        }

        if (isspace((unsigned char)c)) {
            if (had_content) {
                push_token(out_tokens, &cur);
                had_content = 0;
            }

            i++;
            continue;
        }

        if (c == '|') {
            if (had_content) {
                push_token(out_tokens, &cur);
                had_content = 0;
            }

            push_operator(out_tokens, "|");
            i++;
            continue;
        }

        if (c == '&') {
            if (had_content) {
                push_token(out_tokens, &cur);
                had_content = 0;
            }

            push_operator(out_tokens, "&");
            i++;
            continue;
        }

        if (c == '2' && i + 1 < n && line[i + 1] == '>') {
            if (had_content) {
                push_token(out_tokens, &cur);
                had_content = 0;
            }

            if (i + 2 < n && line[i + 2] == '>') {
                push_operator(out_tokens, "2>>");
                i += 3;
            } else {
                push_operator(out_tokens, "2>");
                i += 2;
            }

            continue;
        }

        if (c == '<' || c == '>') {
            if (had_content) {
                push_token(out_tokens, &cur);
                had_content = 0;
            }

            if (c == '>' && i + 1 < n && line[i + 1] == '>') {
                push_operator(out_tokens, ">>");
                i += 2;
            } else if (c == '<') {
                push_operator(out_tokens, "<");
                i++;
            } else {
                push_operator(out_tokens, ">");
                i++;
            }

            continue;
        }

        ds_append_char(&cur, c);
        had_content = 1;
        i++;
    }

    if (had_content)
        push_token(out_tokens, &cur);

    ds_free(&cur);
    return 0;
}
