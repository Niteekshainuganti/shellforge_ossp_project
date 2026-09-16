/* exec.h — Weeks 4 & 7: Process control + pipes */
#ifndef SHELLFORGE_EXEC_H
#define SHELLFORGE_EXEC_H

#include "parser.h"

void run_pipeline(const Pipeline *pipeline,
                  int *last_status,
                  int *should_exit);

#endif /* SHELLFORGE_EXEC_H */
