/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */
#include "ps1.h"

const char *TRAILING_DIAMOND;
const char *LEADING_DIAMOND;
const char *TRAILING_POWERLINE;
const char *LEADING_POWERLINE;

static PS1_block_t *PS1_blocks = NULL;


void prompt_symbols_init() {
    if (shell_is_utf8_supported() && USE_UTF8_SYMBOLS) {
        TRAILING_DIAMOND = "\uE0B4";
        LEADING_DIAMOND = "\uE0B6";
        TRAILING_POWERLINE = "\uE0B0";
        LEADING_POWERLINE = "\uE0B1";
    } else {
        TRAILING_DIAMOND = "[v]";
        LEADING_DIAMOND = "[^]";
        TRAILING_POWERLINE = "[>]";
        LEADING_POWERLINE = "[<]";
    }

    // Define PS1 blocks
    PS1_blocks = xcalloc(4, sizeof(PS1_block_t));
    PS1_blocks[0] = (PS1_block_t){ .text = "\\u@\\h", .leading = NULL, .trailing = TRAILING_DIAMOND, .color = { .fg = NULL, .bg = BG_COLOR_BLUE } };
    PS1_blocks[1] = (PS1_block_t){ .text = "\\W",     .leading = NULL, .trailing = TRAILING_DIAMOND, .color = { .fg = NULL, .bg = BG_COLOR_GREEN } };
    PS1_blocks[2] = (PS1_block_t){ .text = NULL };
}



static int extract_code(const char *seq) {
    if (!seq) return -1;
    int code = -1;
    sscanf(seq, "\033[%dm", &code);
    return code;
}

static inline char *get_reverse_color(const char *color) {
    if (!color) return NULL;

    int code = extract_code(color);
    if (code < 0) return xstrdup(color);

    // If foreground (30–37 or 90–97), convert into background (40–47 or 100–107)
    if ((code >= 30 && code <= 37) || (code >= 90 && code <= 97)) {
        code += 10;
    }
    // If background (40–47 or 100–107), convert into foreground (30–37 or 90–97)
    else if ((code >= 40 && code <= 47) || (code >= 100 && code <= 107)) {
        code -= 10;
    }

    char *result = xmalloc(16);
    snprintf(result, 16, "\033[%dm", code);
    return result;
}

static char *make_ansi_color(const char *fg, const char *bg) {
    int fg_code = extract_code(fg);
    int bg_code = extract_code(bg);

    char buf[32];

    // \001 and \002 are mandatory to inform readline about non-printable characters
    if (fg_code != -1 && bg_code != -1)
        snprintf(buf, sizeof(buf), "\001\033[%d;%dm\002", fg_code, bg_code);
    else if (fg_code != -1)
        snprintf(buf, sizeof(buf), "\001\033[%dm\002", fg_code);
    else if (bg_code != -1)
        snprintf(buf, sizeof(buf), "\001\033[%dm\002", bg_code);
    else
        buf[0] = '\0';

    return xstrdup(buf);
}

static size_t ps1_apply_color(char *dst, size_t max, const char *text, PS1_color_t color, bool reverse) {
    const char *fg = (reverse) ? get_reverse_color(color.fg) : color.fg;
    const char *bg = (reverse) ? get_reverse_color(color.bg) : color.bg;

    const char *ansi_color = (USE_COLORS) ? make_ansi_color(fg, bg) : xstrdup("");
    if (reverse) {
        free((void*)fg);
        free((void*)bg);
    }

    size_t n = (size_t) snprintf(dst, max, "%s%s%s", ansi_color, text, COLOR_RESET);
    free((void*)ansi_color);
    return n;
}

static char *ps1_handle_text(const char *text) {
    shell_identity_t *sh_identity = shell_state_get_identity();
    char output[1024];
    size_t pos = 0;
    while (*text && pos < sizeof(output) - 1) {
        if (*text == '\\') {
            text++;
            if (!*text) break;
            const char *rep = NULL;
            switch (*text) {
                case 'u': rep = sh_identity->username; break;
                case 'h': rep = sh_identity->hostname; break;
                case 'w': rep = sh_identity->cwd; break;
                case 'W': {
                    const char *cwd = sh_identity->cwd;
                    const char *last = strrchr(cwd, '/');
                    rep = (last && last[1]) ? last + 1 : "/";
                    break;
                }
                case '$': rep = (sh_identity->uid == 0) ? "# " : "$ "; break;
                default:
                    fprintf(stderr, "Unknown PS1 escape \\%c\n", *text);
                    rep = "";
                    break;
            }
            if (rep) {
                size_t rlen = strlen(rep);
                if (pos + rlen >= sizeof(output) - 1) rlen = sizeof(output) - 1 - pos;
                memcpy(output + pos, rep, rlen);
                pos += rlen;
            }
        } else {
            output[pos++] = *text;
        }
        text++;
    }
    output[pos] = '\0';
    return xstrdup(output);
}

char *prompt_build_ps1() {
    char buf[1024] = {0};
    size_t pos = 0;
    bool utf8_supported = shell_is_utf8_supported();

    bool should_reverse = false;
    for (size_t i = 0; PS1_blocks[i].text != NULL; i++) {
        const PS1_block_t *b = &PS1_blocks[i];

        // Leading
        char *leading = (char*) b->leading;
        if (leading == NULL || !utf8_supported) leading = " ";
        else should_reverse = true;

        pos += ps1_apply_color(buf + pos, sizeof(buf) - pos, leading, b->color, should_reverse);

        // Text body
        should_reverse = false;
        char *txt = ps1_handle_text(b->text);
        pos += ps1_apply_color(buf + pos, sizeof(buf) - pos, txt, b->color, false);

        // Trailing
        char *trailing = (char*) b->trailing;
        PS1_color_t trailing_color = b->color;
        if (trailing == NULL || !utf8_supported) {
            trailing = " ";
        } else {
            trailing_color.fg = get_reverse_color(b->color.bg);
            if (PS1_blocks[i + 1].text != NULL) {
                trailing_color.bg = PS1_blocks[i + 1].color.bg;
            } else {
                trailing_color.bg = NULL;
            }
        }
        pos += ps1_apply_color(buf + pos, sizeof(buf) - pos, trailing, trailing_color, should_reverse);
        if (b->trailing != NULL) {
            free((void*)trailing_color.fg);
        }
        free(txt);
    }

    // Final space to separate prompt from command
    buf[pos++] = ' ';
    buf[pos] = '\0';
    return xstrdup(buf);
}