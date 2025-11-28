/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */
#ifndef __NOVASH_EXPANDER_PIPELINE_H__
#define __NOVASH_EXPANDER_PIPELINE_H__

#include <ctype.h>
#include <glob.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer/lexer.h"
#include "shell/state.h"
#include "utils/collections.h"
#include "utils/system/memory.h"

char **expand_argv_parts(word_part_t **argv_parts);
char *expand_redirection_target(word_part_t *redir_target_parts);

#endif // __NOVASH_EXPANDER_PIPELINE_H__