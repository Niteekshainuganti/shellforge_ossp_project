#include "dvector.h"

#include <stdio.h>
#include <stdlib.h>

#define DVECTOR_INITIAL_CAP 8

void dv_init(DVector *v) {
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

void dv_free(DVector *v) {
    free(v->items);
    v->items = NULL;
    v->len = 0;
    v->cap = 0;
}

void dv_free_deep(DVector *v) {
    for (size_t i = 0; i < v->len; i++) {
        free(v->items[i]);
    }
    dv_free(v);
}

void dv_push(DVector *v, void *item) {
    if (v->len == v->cap) {
        size_t new_cap = v->cap == 0 ? DVECTOR_INITIAL_CAP : v->cap * 2;
        void **new_items = realloc(v->items, new_cap * sizeof(void *));
        if (!new_items) {
            fprintf(stderr, "shellforge: out of memory (dvector grow)\n");
            exit(1);
        }
        v->items = new_items;
        v->cap = new_cap;
    }
    v->items[v->len++] = item;
}

void *dv_get(const DVector *v, size_t i) {
    if (i >= v->len) return NULL;
    return v->items[i];
}

size_t dv_len(const DVector *v) {
    return v->len;
}
