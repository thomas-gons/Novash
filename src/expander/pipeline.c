/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */
#include "pipeline.h"

static char *expand_special_one(char sigil) {
  shell_state_t *sh = shell_state_get();
  char buf[32];

  switch (sigil) {
  case '?': {
    int st = 0;
    if (sh->last_exec.exit_status >= 0)
      st = sh->last_exec.exit_status;
    snprintf(buf, sizeof(buf), "%d", st);
    return xstrdup(buf);
  }
  case '$': {
    snprintf(buf, sizeof(buf), "%d", sh->identity.pid);
    return xstrdup(buf);
  }
  case '!': {
    snprintf(buf, sizeof(buf), "%d", sh->last_exec.bg_pid);
    return xstrdup(buf);
  }
  case '-': {
    // No need to duplicate as get_flags already does
    return shell_state_get_flags();
  }
  default:
    return NULL;
  }
}

static char *expand_params_in_string(word_part_t in_part) {
  const char *p = in_part.value;
  // Handle $?, $$, $!, $-
  if (*p == '?' || *p == '$' || *p == '!' || *p == '-') {
    if (strlen(p) > 1)
      pr_err("expander: invalid parameter expansion: $%s\n", p);

    return expand_special_one(*p);
  }

  // Handle $VAR_NAME and ${VAR_NAME} lexer returns only VAR_NAME part
  if (isalpha((unsigned char)*p) || *p == '_') {
    char *val = shell_state_getenv(p);
    return xstrdup(val ? val : "");
  }

  pr_err("expander: invalid parameter expansion: $%s\n", p);
  return in_part.value;
}

static void pass_expand_params(word_part_t *in_parts) {
  for (int i = 0; i < arrlen(in_parts); i++) {
    word_part_t *wp = &in_parts[i];
    if (wp->type != WORD_VARIABLE)
      continue;
    wp->value = expand_params_in_string(*wp);
    wp->type = WORD_LITERAL;
  }
}

static char *expand_tilde_str(const char *s) {
  const char *user = s + 1;
  size_t user_len = strlen(user);

  if (user_len == 0) {
    return xstrdup(getenv("HOME"));
  } else {
    if (user_len > 256)
      return NULL;
    struct passwd *pw = getpwnam(user);
    if (!pw || !pw->pw_dir)
      return NULL;
    return xstrdup(pw->pw_dir);
  }
}

static int pass_expand_tilde(word_part_t *in_parts) {
  for (int i = 0; i < arrlen(in_parts); i++) {
    word_part_t *wp = &in_parts[i];
    if (wp->type != WORD_TILDE)
      continue;
    char *expanded = expand_tilde_str(wp->value);
    if (!expanded) {
      nsh_msg("user not found for '%s'\n", wp->value);
      return -1;
    }
    wp->type = WORD_LITERAL;
    wp->value = expanded;
  }
  return 0;
}

static char *build_str_from_parts(word_part_t *parts) {
  size_t total_len = 0;
  for (int i = 0; i < arrlen(parts); i++)
    total_len += strlen(parts[i].value);
  total_len += 1;

  char *buf = xmalloc(total_len);
  buf[0] = '\0';
  for (int i = 0; i < arrlen(parts); i++)
    strcat(buf, parts[i].value);

  return buf;
}

static char **pass_glob(word_part_t *in_parts) {
  char **out = NULL;
  glob_t g = {0};

  char *pattern = build_str_from_parts(in_parts);
  int rc = glob(pattern, 0, NULL, &g);
  if (rc == GLOB_NOMATCH || g.gl_pathc == 0) {
    nsh_msg("no matches found for pattern '%s'\n", pattern);
    free(pattern);
    globfree(&g);
    arrfree(out);
    return NULL;
  } else {
    for (size_t j = 0; j < g.gl_pathc; j++)
      arrpush(out, xstrdup(g.gl_pathv[j]));
  }
  free(pattern);
  globfree(&g);
  return out;
}

char **expand_argv_parts(word_part_t **argv_parts, bool *invalid) {
  char **argv = NULL;
  for (int i = 0; i < arrlen(argv_parts); i++) {
    word_part_t *parts = argv_parts[i];

    pass_expand_params(parts);
    if (pass_expand_tilde(parts) != 0) {
      *invalid = true;
      arrfree(argv);
      return NULL;
    }

    if (arrlen(parts) == 1 && parts[0].type == WORD_LITERAL) {
      arrpush(argv, xstrdup(parts[0].value));
    } else {
      char **argv_entry = pass_glob(parts);
      if (!argv_entry) {
        *invalid = true;
        arrfree(argv);
        return NULL;
      }

      for (int j = 0; j < arrlen(argv_entry); j++) {
        arrpush(argv, xstrdup(argv_entry[j]));
        free(argv_entry[j]);
      }
      arrfree(argv_entry);
    }
  }
  arrpushnc(argv, NULL);
  return argv;
}

char *expand_redirection_target(word_part_t *redir_target_parts,
                                bool *invalid) {
  pass_expand_params(redir_target_parts);
  if (pass_expand_tilde(redir_target_parts) != 0) {
    *invalid = true;
    return NULL;
  }

  char *final_str = build_str_from_parts(redir_target_parts);
  return final_str;
}