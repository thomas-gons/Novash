/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "tokenizer.h"

tokenizer_t *tokenizer_new() {
    tokenizer_t *tz = xmalloc(sizeof(tokenizer_t));
    tz->input = NULL;
    tz->pos = 0;
    tz->length = 0;
    return tz;
}

void tokenizer_init(tokenizer_t *tz, char *input) {
    tz->input = xstrdup(input);
    tz->pos = 0;
    tz->length = strlen(input);
}

void tokenizer_free(tokenizer_t *tz) {
    if (tz) {
        if (tz->input) free(tz->input);
        free(tz);
    }
}

void tokenizer_free_token(token_t *tok) {
    if (tok && tok->value) {
        free(tok->value);
        tok->value = NULL;
    }
}

/**
 * peek the current character without advancing the position
 * @param tz pointer to the tokenizer
 * @return current character or '\0' if at the end of input
 */
static char peek(tokenizer_t *tz) {
    if (tz->pos >= tz->length) return '\0';
    return tz->input[tz->pos];
}

/**
 * advance the position and return the current character
 * @param tz pointer to the tokenizer
 * @return current character or '\0' if at the end of input
 */
static char advance(tokenizer_t *tz) {
    if (tz->pos >= tz->length) return '\0';
    return tz->input[tz->pos++];
}

/**
 * skip all forward whitespaces and tabs
 * @param tz pointer to the tokenizer
 */
static void skip_whitespaces(tokenizer_t *tz) {
    char c;
    while ((c = peek(tz)) == ' ' || c == '\t') {
        advance(tz);
    }
}

/**
 * check if a character is a metacharacter
 * @param c character to check
 * @return true if the character is a metacharacter, false otherwise
 */
static bool is_metachar(char c) {
    return (c == '|' || c == '&' || c == ';' || c == '<' || c == '>');
}
/**
 * @brief Extracts a word token, handling nested single ('') and double ("") quotes.
 * Quotes are consumed from the input stream and *removed* from the resulting 'buf', 
 * but are included in the 'raw_length' count. Extraction stops upon 
 * unquoted whitespace or an unquoted metacharacter.
 *
 * @param tz Pointer to the tokenizer state.
 * @param buf Buffer to store the processed (unquoted) word value.
 * @param buf_cap Initial capacity of the buffer.
 * @return The total **raw length** (including quotes) of the word consumed.
 */
static size_t handle_word_token(tokenizer_t *tz, char *buf, size_t buf_cap) {
    size_t length = 0;
    size_t raw_length = 0;

    // flags for single and double quotes
    bool in_sq = false, in_dq = false;

    char c;
    // loop until reaching end of input, whitespace, or metacharacter
    while ((c = peek(tz)) != '\0') {
        if (c == '\'' && !in_dq) { in_sq = !in_sq; advance(tz); raw_length++; continue; }
        if (c == '\"' && !in_sq) { in_dq = !in_dq; advance(tz); raw_length++; continue; }
        if (!in_sq && !in_dq && ((c == ' ' || c == '\t') || is_metachar(c))) break;
        
        // resize buffer if needed (double its size)
        if (length == buf_cap) { buf_cap *= 2; buf = (char*) xrealloc(buf, buf_cap);}
        buf[length++] = advance(tz);
        raw_length++;
    }

    // if buffer is full, expand it by one for null terminator
    if (length == buf_cap) { buf_cap++; buf = (char*) xrealloc(buf, buf_cap);}
    buf[length] = '\0';

    return raw_length;
}

token_t tokenizer_next_token(tokenizer_t *tz) {

    skip_whitespaces(tz);
    char c = peek(tz);
    switch (c) {
        case '|': {
            advance(tz);
            if (peek(tz) == '|') { advance(tz); return (token_t){TOK_OR, NULL, 2}; }
            else return (token_t){TOK_PIPE, NULL, 1};
        }
        case '&': {
            advance(tz);
            if (peek(tz) == '&') { advance(tz); return (token_t){TOK_AND, NULL, 2}; }
            else return (token_t){TOK_BG, NULL, 1};
        }
        case '>': {
            advance(tz);
            if (peek(tz) == '>') { advance(tz); return (token_t){TOK_REDIR_APPEND, NULL, 2}; }
            else return (token_t){TOK_REDIR_OUT, NULL, 1};
        }
        case '<': { advance(tz); return (token_t){TOK_REDIR_IN, NULL, 1}; }
        case ';': { advance(tz); return (token_t) {TOK_SEMI, NULL, 1}; }
        case '\0': { advance(tz); return (token_t) {TOK_EOF, NULL, 0}; }
        default: {
            
            size_t buf_cap = 64;
            char *buf = xmalloc(buf_cap);
            size_t raw_length = handle_word_token(tz, buf, buf_cap);

            // Check for File Descriptor (FD) token:
            // An FD is recognized only if the token is a bare, unquoted number
            // immediately preceding a redirection metacharacter (< or >).
            char *end;
            strtol(buf, &end, 10);
            if (*end == '\0' && strlen(buf) == raw_length) {
                c = peek(tz);
                if (c == '>' || c == '<') return (token_t) {TOK_FD, buf, raw_length};
            }
            
            return (token_t) {TOK_WORD, buf, raw_length}; 
        }
    }
}


size_t tokenizer_token_str(token_t tok, char *buf, size_t buf_sz) {
    char *fixed = NULL;
    char *fmt = NULL;
    int needed;

    switch (tok.type) {
    case TOK_AND:          fixed = "[TOK_AND]: &&\n"; break;
    case TOK_BG:           fixed = "[TOK_BG]: &\n"; break;
    case TOK_EOF:          fixed = "[TOK_EOF]\n"; break;
    case TOK_OR:           fixed = "[TOK_OR]: ||\n"; break;
    case TOK_PIPE:         fixed = "[TOK_PIPE]: |\n"; break;
    case TOK_REDIR_APPEND: fixed = "[TOK_REDIR_APPEND]: >>\n"; break;
    case TOK_REDIR_IN:     fixed = "[TOK_REDIR_IN]: <\n"; break;
    case TOK_REDIR_OUT:    fixed = "[TOK_REDIR_OUT]: >\n"; break;
    case TOK_SEMI:         fixed = "[TOK_SEMI]: ;\n"; break;
    case TOK_FD:
        if (!tok.value)    fixed = "[TOK_FD]: (null)\n";
        else               fmt = "[TOK_FD]: %s\n";
        break;        
    case TOK_WORD:
        if (!tok.value)    fixed = "[TOK_WORD]: (null)\n";
        else               fmt = "[TOK_WORD]: %s\n";
        break;
    default:               fixed = "[UNKNOWN]: (unknown token)\n"; break;
    }

    if (fixed) {
        size_t len = strlen(fixed);
        if (buf && buf_sz > 0) {
            // directly copy fixed string into buffer with size limit
            size_t to_copy = (len >= buf_sz) ? (buf_sz - 1) : len;
            memcpy(buf, fixed, to_copy);
            buf[to_copy] = '\0';
        }
        return len;
    }

    needed = xsnprintf(NULL, 0, fmt, tok.value);
    if (buf && buf_sz > 0) {
        // write formatted string into buffer with size limit
        xsnprintf(buf, buf_sz, fmt, tok.value);
    }
    return (size_t) needed;
}