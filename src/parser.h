/* parser.h — Pipeline AST + I/O redirection */
#ifndef SHELLFORGE_PARSER_H
#define SHELLFORGE_PARSER_H

#include "dvector.h"

typedef struct {
    char **argv;
    int argc;

    char *input_file;
    char *output_file;
    char *error_file;

    int append_output;
    int append_error;

    int background;
} Command;

typedef struct {
    DVector commands;
} Pipeline;

int parse_pipeline(const DVector *tokens,
                   Pipeline *pipeline_out,
                   const char **err_msg);

void pipeline_free(Pipeline *p);

#endif /* SHELLFORGE_PARSER_H */
