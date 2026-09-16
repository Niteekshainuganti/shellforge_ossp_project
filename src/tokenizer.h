#ifndef SHELLFORGE_TOKENIZER_H
#define SHELLFORGE_TOKENIZER_H

#include "dvector.h"

int tokenize(const char *line, DVector *out_tokens, const char **err_msg);

#endif /* SHELLFORGE_TOKENIZER_H */
