#include "dstring.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DSTRING_INITIAL_CAP 32

static void ds_grow_to(DString *s, size_t needed_cap) {
    if (needed_cap <= s->cap) return;

    size_t new_cap = s->cap == 0 ? DSTRING_INITIAL_CAP : s->cap;
    while (new_cap < needed_cap) {
        new_cap *= 2;
    }

    char *new_data = realloc(s->data, new_cap);
    if (!new_data) {
        fprintf(stderr, "shellforge: out of memory (dstring grow)\n");
        exit(1);
    }

    s->data = new_data;
    s->cap = new_cap;
}

void ds_init(DString *s) {
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
    ds_grow_to(s, DSTRING_INITIAL_CAP);
    s->data[0] = '\0';
}

void ds_free(DString *s) {
    free(s->data);
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
}

void ds_clear(DString *s) {
    s->len = 0;
    if (s->data) s->data[0] = '\0';
}

void ds_append_char(DString *s, char c) {
    ds_grow_to(s, s->len + 2);
    s->data[s->len++] = c;
    s->data[s->len] = '\0';
}

void ds_append_str(DString *s, const char *text) {
    size_t add = strlen(text);
    ds_grow_to(s, s->len + add + 1);
    memcpy(s->data + s->len, text, add);
    s->len += add;
    s->data[s->len] = '\0';
}

const char *ds_cstr(const DString *s) {
    return s->data ? s->data : "";
}

