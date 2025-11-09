#ifndef __NOVASH_EXPANDER_PIPELINE_H__
#define __NOVASH_EXPANDER_PIPELINE_H__


#include <glob.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "shell/state.h"
#include "lexer/lexer.h"
#include "utils/collections.h"
#include "utils/system/memory.h"


char **expand_argv_parts(word_part_t **argv_parts);
char *expand_redirection_target(word_part_t *redir_target_parts);

#endif // __NOVASH_EXPANDER_PIPELINE_H__