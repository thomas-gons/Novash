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

void tokenizer_free(tokenizer_t *tz) {
    if (tz) {
        if (tz->input) free(tz->input);
        free(tz);
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
 * handle word token extraction
 * @param tz pointer to the tokenizer
 * @return extracted word token string
 */
static char *handle_word_token(tokenizer_t *tz) {

    size_t buf_cap = 64;
    char *buf = xmalloc(buf_cap);
    size_t length = 0;

    // flags for single and double quotes
    bool in_sq = false, in_dq = false;

    char c;
    // loop until reaching end of input, whitespace, or metacharacter
    while ((c = peek(tz)) != '\0') {
        if (c == '\'' && !in_dq) { in_sq = !in_sq; advance(tz); continue; }
        if (c == '\"' && !in_sq) { in_dq = !in_dq; advance(tz); continue; }
        if (!in_sq && !in_dq && ((c == ' ' || c == '\t') || is_metachar(c))) break;
        
        // resize buffer if needed (double its size)
        if (length == buf_cap) { buf_cap *= 2; buf = (char*) xrealloc(buf, buf_cap);}
        buf[length++] = advance(tz);
    }

    // if buffer is full, expand it by one for null terminator
    if (length == buf_cap) { buf_cap++; buf = (char*) xrealloc(buf, buf_cap);}
    buf[length] = '\0';

    return buf;
}

token_t tokenizer_next(tokenizer_t *tz) {

    skip_whitespaces(tz);
    char c = peek(tz);
    switch (c) {
        case '|': {
            advance(tz);
            if (peek(tz) == '|') { advance(tz); return (token_t){TOK_OR, NULL}; }
            else return (token_t){TOK_PIPE, NULL};
        }
        case '&': {
            advance(tz);
            if (peek(tz) == '&') { advance(tz); return (token_t){TOK_AND, NULL}; }
            else return (token_t){TOK_BG, "&"};
        }
        case '>': {
            advance(tz);
            if (peek(tz) == '>') { advance(tz); return (token_t){TOK_REDIR_APPEND, NULL}; }
            else return (token_t){TOK_REDIR_OUT, NULL};
        }
        case '<': { advance(tz); return (token_t){TOK_REDIR_IN, NULL}; }
        case ';': { advance(tz); return (token_t) {TOK_SEMI, NULL}; }
        case '\0': { advance(tz); return (token_t) {TOK_EOF, NULL}; }
        default: {
            char *word = handle_word_token(tz);

            // the word might be a file descriptor if it's all digits
            char *end;
            strtol(word, &end, 10);
            if (*end == '\0') {
                c = peek(tz);
                // ensure next char is a redirection metacharacter
                // only n>, n< or n>> are valid fd tokens
                if (c == '>' || c == '<') return (token_t) {TOK_FD, word};
            }
            
            return (token_t) {TOK_WORD, word}; 
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