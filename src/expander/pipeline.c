#include "pipeline.h"


static bool has_glob_meta(const char *s) {
    return s && strpbrk(s, "*?[") != NULL;
}

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
        if (wp->type != WORD_VARIABLE) continue;
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
        if (user_len > 256) return NULL;
        struct passwd *pw = getpwnam(user);
        if (!pw || !pw->pw_dir) return NULL;
        return xstrdup(pw->pw_dir);
    }
}

static void pass_expand_tilde(word_part_t *in_parts) {
    for (int i = 0; i < arrlen(in_parts); i++) {
        word_part_t *wp = &in_parts[i];
        if (wp->type != WORD_TILDE) continue;
        wp->value = expand_tilde_str(wp->value);
        wp->type = WORD_LITERAL;
    }
}

static char *build_globbed_string(word_part_t *parts, int part_index) {
    size_t total_len = 0;

    if (part_index > 0) {
        total_len += strlen(parts[part_index - 1].value);
    }

    total_len += strlen(parts[part_index].value);

    if (part_index + 1 < arrlen(parts)) {
        total_len += strlen(parts[part_index + 1].value);
    }

    total_len += 1;

    char *buf = xmalloc(total_len);
    buf[0] = '\0';

    if (part_index > 0) strcat(buf, parts[part_index - 1].value);
    strcat(buf, parts[part_index].value);
    if (part_index + 1 < arrlen(parts)) strcat(buf, parts[part_index + 1].value);

    return buf;
}

static word_part_t *pass_glob(word_part_t *in_parts) {
    for (int i = 0; i < arrlen(in_parts); i++) {
        word_part_t *wp = &in_parts[i];
        if (wp->type != WORD_GLOB) continue;
        
        if (wp->quote != QUOTE_SINGLE && has_glob_meta(wp->value)) {
            glob_t g = {0};

            char *pattern = build_globbed_string(in_parts, i);
            int rc = glob(pattern, 0, NULL, &g);
            if (rc == 0 && g.gl_pathc > 0) {
                // For simplicity, we only handle the first match here.
                for (size_t j = 0; j < g.gl_pathc; j++) {
                    wp->value = xstrdup(g.gl_pathv[j]);
                    break;
                }
                globfree(&g);
                continue;
            }
            globfree(&g);
        }
    }
    word_part_t *out = NULL;
    int parts_len = (int) arrlen(in_parts);
    for (int i = 0; i < parts_len; i++) {
        if (i + 1 < parts_len && in_parts[i+1].type == WORD_GLOB) {
            continue;
        }
        if (i - 1 >= 0 && in_parts[i-1].type == WORD_GLOB) {
            continue;
        }

        arrpush(out, in_parts[i]);
    }

    return out;
}

static char *build_final_string_from_parts(word_part_t *parts) {
    size_t total_len = 0;
    for (int i = 0; i < arrlen(parts); i++) {
        total_len += strlen(parts[i].value);
    }

    char *result = xmalloc(total_len + 1);
    size_t pos = 0;
    for (int i = 0; i < arrlen(parts); i++) {
        size_t part_len = strlen(parts[i].value);
        memcpy(result + pos, parts[i].value, part_len);
        pos += part_len;
    }
    result[total_len] = '\0';
    return result;
}

char *expand_run_pipeline(word_part_t *in_parts) {
    pass_expand_params(in_parts);
    pass_expand_tilde(in_parts);
    word_part_t *out_parts = pass_glob(in_parts);

    return build_final_string_from_parts(out_parts);
}