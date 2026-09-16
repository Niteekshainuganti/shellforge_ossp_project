#ifndef SHELLFORGE_DVECTOR_H
#define SHELLFORGE_DVECTOR_H

#include <stddef.h>

typedef struct {
    void  **items;
    size_t  len;
    size_t  cap;
} DVector;

void   dv_init(DVector *v);
void   dv_free(DVector *v);
void   dv_free_deep(DVector *v);
void   dv_push(DVector *v, void *item);
void  *dv_get(const DVector *v, size_t i);
size_t dv_len(const DVector *v);

#endif /* SHELLFORGE_DVECTOR_H */
