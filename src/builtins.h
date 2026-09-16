/* builtins.h — Week 5: PATH + built-ins, exit codes */
#ifndef SHELLFORGE_BUILTINS_H
#define SHELLFORGE_BUILTINS_H

#include "parser.h"

int is_builtin(const Command *cmd);

void run_builtin(const Command *cmd,
                 int *last_status,
                 int *should_exit);

#endif /* SHELLFORGE_BUILTINS_H */
