/* parser.h — Week 3: Pipeline AST */
#ifndef SHELLFORGE_PARSER_H
#define SHELLFORGE_PARSER_H

#include "dvector.h"

typedef struct {
    char **argv;
    int argc;
} Command;

typedef struct {
    DVector commands;
} Pipeline;

int parse_pipeline(const DVector *tokens, Pipeline *pipeline_out,
                   const char **err_msg);

void pipeline_free(Pipeline *p);

#endif /* SHELLFORGE_PARSER_H */
