#include "pipeline.h"


static bool has_glob_meta(const char *s) {
    return s && strpbrk(s, "*?[") != NULL;
}

static char *dup_shell_flag_string(shell_state_t *sh) {
    char *f = shell_state_get_flags();
    return f ? f : xstrdup("");
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
            return dup_shell_flag_string(sh);
        }
        default:
            return NULL;
    }
}


static char *expand_params_in_string(word_part_t *in_part) {
    size_t cap = strlen(in_part->value) + 32;
    size_t len = 0;
    char *out = xmalloc(cap);

    const char *p = in_part->value;
    while (*p) {
        if (*p != '$') {
            // copy plain char
            if (len + 2 > cap) { cap *= 2; out = xrealloc(out, cap); }
            out[len++] = *p++;
            continue;
        }

        p++;
        if (*p == '\0') {
            if (len + 2 > cap) { cap *= 2; out = xrealloc(out, cap); }
            out[len++] = '$';
            break;
        }

        // Handle $?, $$, $!, $-
        if (*p == '?' || *p == '$' || *p == '!' || *p == '-') {
            char *rep = expand_special_one(*p);
            size_t rlen = strlen(rep);
            if (len + rlen + 1 > cap) { cap = (len + rlen + 1) * 2; out = xrealloc(out, cap); }
            memcpy(out + len, rep, rlen);
            len += rlen;
            free(rep);
            p++;
            continue;
        }

        // Handle $VAR_NAME
        if (isalpha((unsigned char)*p) || *p == '_') {
            const char *start = p;
            p++;
            while (isalnum((unsigned char)*p) || *p == '_') p++;
            size_t vlen = (size_t)(p - start);
            char *vname = xstrdup_n(start, vlen);
            char *rep = xstrdup(shell_state_getenv(vname));
            free(vname);

            size_t rlen = strlen(rep);
            if (len + rlen + 1 > cap) { cap = (len + rlen + 1) * 2; out = xrealloc(out, cap); }
            memcpy(out + len, rep, rlen);
            len += rlen;
            free(rep);
            continue;
        }

        if (len + 2 > cap) { cap *= 2; out = xrealloc(out, cap); }
        out[len++] = '$';
        if (*p) {
            if (len + 2 > cap) { cap *= 2; out = xrealloc(out, cap); }
            out[len++] = *p++;
        }
    }

    out[len] = '\0';
    return out;
}

static void pass_expand_params(word_part_t *in_parts) {
    for (int i = 0; i < arrlen(in_parts); i++) {
        word_part_t *wp = &in_parts[i];
        if (wp->type == WORD_LITERAL) continue;
        wp->value = expand_params_in_string(wp);
    }
}

static char *expand_tilde_str(const char *s) {
    const char *slash = strchr(s, '/');
    const char *user = s + 1;
    size_t user_len = (slash ? (size_t)(slash - user) : strlen(user));

    if (user_len == 0) {
        const char *home = getenv("HOME");
        if (!home) return NULL;
        if (!slash) return xstrdup(home);
        size_t len = strlen(home) + strlen(slash) + 1;
        char *r = xmalloc(len);
        snprintf(r, len, "%s%s", home, slash);
        return r;
    } else {
        if (user_len > 256) return NULL;
        char uname[257];
        memcpy(uname, user, user_len);
        uname[user_len] = '\0';
        struct passwd *pw = getpwnam(uname);
        if (!pw || !pw->pw_dir) return NULL;
        if (!slash) return xstrdup(pw->pw_dir);
        size_t len = strlen(pw->pw_dir) + strlen(slash) + 1;
        char *r = xmalloc(len);
        snprintf(r, len, "%s%s", pw->pw_dir, slash);
        return r;
    }
}

static void pass_expand_tilde(word_part_t *in_parts) {
    
    for (int i = 0; i < arrlen(in_parts); i++) {
        word_part_t *wp = &in_parts[i];
        if (wp->type == WORD_LITERAL) continue;

        if (wp->quote != QUOTE_SINGLE && wp->value[0] == '~') {
            wp->value = expand_tilde_str(wp->value);
        }
    }
}

static void pass_glob(word_part_t *in_parts) {

    for (int i = 0; i < arrlen(in_parts); i++) {
        word_part_t *wp = &in_parts[i];
        if (wp->type == WORD_LITERAL) continue;
        
        if (wp->quote != QUOTE_SINGLE && has_glob_meta(wp->value)) {
            glob_t g = {0};
            int rc = glob(wp->value, 0, NULL, &g);
            if (rc == 0 && g.gl_pathc > 0) {
                for (size_t k = 0; k < g.gl_pathc; k++) {
                    wp->value = xstrdup(g.gl_pathv[k]);
                }
                globfree(&g);
                continue;
            }
            globfree(&g);
        }

    }
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
    pass_glob(in_parts);

    return build_final_string_from_parts(in_parts);
}