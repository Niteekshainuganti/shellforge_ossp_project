#ifndef SHELLFORGE_DSTRING_H
#define SHELLFORGE_DSTRING_H

#include <stddef.h>

typedef struct {
    char   *data;
    size_t  len;
    size_t  cap;
} DString;

void ds_init(DString *s);
void ds_free(DString *s);
void ds_clear(DString *s);
void ds_append_char(DString *s, char c);
void ds_append_str(DString *s, const char *text);
const char *ds_cstr(const DString *s);

#endif /* SHELLFORGE_DSTRING_H */
#ifndef SHELLFORGE_DSTRING_H
#define SHELLFORGE_DSTRING_H

#include <stddef.h>

typedef struct {
    char   *data;
    size_t  len;
    size_t  cap;
} DString;

void ds_init(DString *s);
void ds_free(DString *s);
void ds_clear(DString *s);
void ds_append_char(DString *s, char c);
void ds_append_str(DString *s, const char *text);
const char *ds_cstr(const DString *s);

#endif /* SHELLFORGE_DSTRING_H */

