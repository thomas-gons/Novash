/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "lexer.h"

lexer_t *lexer_new() {
  lexer_t *lex = xmalloc(sizeof(lexer_t));
  lex->input = NULL;
  lex->pos = 0;
  lex->length = 0;
  return lex;
}

void lexer_init(lexer_t *lex, char *input) {
  if (lex->input)
    free(lex->input);

  lex->input = xstrdup(input);
  lex->pos = 0;
  lex->length = strlen(input);
}

void lexer_free(lexer_t *lex) {
  if (!lex)
    return;

  if (lex->input)
    free(lex->input);
  free(lex);
}

void lexer_free_token(token_t *tok) {
  if (tok && tok->raw_value) {
    free(tok->raw_value);
    for (int i = 0; i < arrlen(tok->parts); i++) {
      free(tok->parts[i].value);
    }
    arrfree(tok->parts);
    tok->raw_value = NULL;
  }
}

static inline char peek(lexer_t *lex) {
  return (lex->pos < lex->length) ? lex->input[lex->pos] : '\0';
}

static inline char advance(lexer_t *lex) {
  return (lex->pos < lex->length) ? lex->input[lex->pos++] : '\0';
}

static inline bool ismetachar(char c) {
  switch (c) {
    case '|':
    case '&':
    case ';':
    case '<':
    case '>':
    case '$':
      return true;
    default:
      return false;
  }
}

#define is_meta_char(c) (c == '|' || c == '&' || c == ';' || c == '<' || c == '>')
#define is_expansion_char(c) (c == '$' || c == '*' || c == '?' || c == '~')
#define is_special_parameter_char(c) ((c) == '$' || (c) == '?' || (c) == '!' || (c) == '-')
#define is_word_char(c) (!isspace(c) && !is_meta_char(c) && !is_expansion_char(c))


static inline void skip_whitespaces(lexer_t *lex) {
  char c;
  while ((c = peek(lex)) != '\0' && isspace(c)) {
    advance(lex);
  }
}

static inline bool is_word_fd(char *buf) {
  char *end;
  strtol(buf, &end, 10);
  return *end == '\0' && strlen(buf) > 0;
}

// only handles simple escapes like \n, \t, \\, \', \"
static char handle_escape(lexer_t *lex) {
  advance(lex); // skip '\'
  
  char c = advance(lex);
  switch (c) {
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'v': return '\v';
    case '\\': return '\\';
    case '\'':  return '\'';
    case '\"': return '\"';
    default:
      return c;
  }
}

static char *handle_literal(lexer_t *lex, quote_context_e quote_ctx) {
    size_t buf_cap = 64;
    size_t buf_len = 0;
    char *buf = xmalloc(buf_cap);

    while (1) {
        char c = peek(lex);
        if (c == '\0') break;

        if (quote_ctx == QUOTE_NONE) {
            if (!is_word_char(c) || c == '\'' || c == '\"') break;
        } else if (quote_ctx == QUOTE_SINGLE && c == '\'') {
            break;
        } else if (quote_ctx == QUOTE_DOUBLE && (c == '\"' || c == '$')) {
            break;
        }

        if (c == '\\' && quote_ctx != QUOTE_SINGLE) {
            char esc = handle_escape(lex);
            if (buf_len + 1 >= buf_cap) {
                buf_cap *= 2;
                buf = xrealloc(buf, buf_cap);
            }
            buf[buf_len++] = esc;
        } else {
            if (buf_len + 1 >= buf_cap) {
                buf_cap *= 2;
                buf = xrealloc(buf, buf_cap);
            }
            buf[buf_len++] = advance(lex);
        }
    }

    buf[buf_len] = '\0';
    return (buf_len > 0) ? buf : NULL;
}

static char *handle_variable_word_part(lexer_t *lex) {
  advance(lex); // skip '$'
  size_t start = lex->pos;
  bool has_curly = false;
  if (peek(lex) == '{') {
    has_curly = true;
    advance(lex);
  }

  if (is_special_parameter_char(peek(lex))) {
    advance(lex);
    if (has_curly) {
      if (peek(lex) == '}')
        advance(lex);
      
      else pr_err("lexer: unmatched '{' in variable name\n");
    }
    return xstrdup_n(&lex->input[start] + has_curly, 1);
  }
  
  while (isalnum(peek(lex)) || peek(lex) == '_') {
    advance(lex);
  }

  if (has_curly) {
    if (peek(lex) == '}')
      advance(lex);
    
    else pr_err("lexer: unmatched '{' in variable name\n");
  }

  size_t len = lex->pos - start - 2 * has_curly;
  // just a '$' considered as literal
  if (len == 0) {

    // check for the case of '${}'
    if (has_curly) return "";
    return NULL;
  }

  return xstrdup_n(&lex->input[start] + has_curly, len);
}

static char *handle_tilde_word_part(lexer_t *lex) {
  size_t start = lex->pos;
  advance(lex); // skip '~'

  while (isalnum(peek(lex)) || peek(lex) == '_') {
    advance(lex);
  }

  size_t len = lex->pos - start;
  return xstrdup_n(&lex->input[start], len);
}

static char *handle_glob_word_part(lexer_t *lex) {
  char c = peek(lex);
  advance(lex); // skip glob character
  return xstrdup_n(&c, 1);
}

static word_part_t lex_next_word_part(lexer_t *lex, quote_context_e *quote_ctx) {
  char c = peek(lex);

  if (c == '\'' || c == '\"') {
    advance(lex);
    *quote_ctx = (*quote_ctx == QUOTE_NONE)
                   ? (c == '\'' ? QUOTE_SINGLE : QUOTE_DOUBLE)
                   : QUOTE_NONE;
    
    return (word_part_t){0};
  }

  // Handle variable expansion
  if (c == '$' && (*quote_ctx == QUOTE_NONE || *quote_ctx == QUOTE_DOUBLE)) {
    char *varname = handle_variable_word_part(lex);

    if (!varname) {
      return (word_part_t){WORD_LITERAL, *quote_ctx, xstrdup("$")};
    }

    // ignore ${} case
    if (*varname == '\0') {
      // varname is stack allocated here (no free needed)
      return (word_part_t){0};
    }
    
    return (word_part_t){WORD_VARIABLE, *quote_ctx, varname};
  }

  // Handle tilde expansion
  if (c == '~' && *quote_ctx == QUOTE_NONE) {
    char *tilde = handle_tilde_word_part(lex);
    return (word_part_t){WORD_TILDE, *quote_ctx, tilde};
  }

  // Handle globbing characters
  // Future note: could be improved to handle [a-z] and {brace} patterns
  if ((c == '*' || c == '?') && *quote_ctx == QUOTE_NONE) {
    char *glob = handle_glob_word_part(lex);
    return (word_part_t){WORD_GLOB, *quote_ctx, glob};
  }

  char *lit = handle_literal(lex, *quote_ctx);
  return (word_part_t){WORD_LITERAL, *quote_ctx, lit ? lit : xstrdup("")};
}

static token_t handle_word_token(lexer_t *lex) {
  word_part_t *parts = NULL;
  quote_context_e quote_ctx = QUOTE_NONE;
  char *raw_start = &lex->input[lex->pos];

  while (1) {
    char c = peek(lex);
    if (c == '\0')
      break;

    if (quote_ctx == QUOTE_NONE && (isspace(c) || is_meta_char(c)))
      break;

    word_part_t part = lex_next_word_part(lex, &quote_ctx);
    if (part.value && strlen(part.value) > 0)
      arrput(parts, part);
  }

  char *raw_value = xstrdup_n(raw_start, (size_t) (&lex->input[lex->pos] - raw_start));

  return (token_t) {
    .type = TOK_WORD,
    .raw_value = raw_value,
    .parts = parts
  };
}

token_t lexer_next_token(lexer_t *lex) {

  skip_whitespaces(lex);
  char c = peek(lex);
  switch (c) {
    case '|': {
      advance(lex);
      if (peek(lex) == '|') {
        advance(lex);
        return (token_t){TOK_OR, NULL, NULL};
      } else
        return (token_t){TOK_PIPE, NULL,NULL};
    }
    case '&': {
      advance(lex);
      if (peek(lex) == '&') {
        advance(lex);
        return (token_t){TOK_AND, NULL,NULL};
      } else
        return (token_t){TOK_BG, NULL,NULL};
    }
    case '>': {
      advance(lex);
      if (peek(lex) == '>') {
        advance(lex);
        return (token_t){TOK_REDIR_APPEND, NULL,NULL};
      } else
        return (token_t){TOK_REDIR_OUT, NULL, NULL};
    }
    case '<': {
      advance(lex);
      return (token_t){TOK_REDIR_IN, NULL, NULL};
    }
    case ';': {
      advance(lex);
      return (token_t){TOK_SEMI, NULL, NULL};
    }
    case '\0': {
      advance(lex);
      return (token_t){TOK_EOF, NULL, NULL};
    }
    default: {
      token_t tok = handle_word_token(lex);
      
      if (is_word_fd(tok.raw_value)) {
        tok.type = TOK_FD;
      }

      return tok;
    }
  }
}

size_t lexer_token_str(token_t tok, char *buf, size_t buf_sz) {
  char temp[1024] = {0};
  size_t offset = 0;

  // -- 1. Fixed tokens without values --
  switch (tok.type) {
  case TOK_AND:           return (size_t) snprintf(buf, buf_sz, "[TOK_AND]: &&\n");
  case TOK_BG:            return (size_t) snprintf(buf, buf_sz, "[TOK_BG]: &\n");
  case TOK_EOF:           return (size_t) snprintf(buf, buf_sz, "[TOK_EOF]\n");
  case TOK_OR:            return (size_t) snprintf(buf, buf_sz, "[TOK_OR]: ||\n");
  case TOK_PIPE:          return (size_t) snprintf(buf, buf_sz, "[TOK_PIPE]: |\n");
  case TOK_REDIR_APPEND:  return (size_t) snprintf(buf, buf_sz, "[TOK_REDIR_APPEND]: >>\n");
  case TOK_REDIR_IN:      return (size_t) snprintf(buf, buf_sz, "[TOK_REDIR_IN]: <\n");
  case TOK_REDIR_OUT:     return (size_t) snprintf(buf, buf_sz, "[TOK_REDIR_OUT]: >\n");
  case TOK_SEMI:          return (size_t) snprintf(buf, buf_sz, "[TOK_SEMI]: ;\n");
  default: break;
  }

  // -- 2. TOK_FD or TOK_WORD --
  if (tok.type == TOK_FD) {
    if (!tok.raw_value)
      return (size_t) snprintf(buf, buf_sz, "[TOK_FD]: (null)\n");
    return (size_t) snprintf(buf, buf_sz, "[TOK_FD]: %s\n", tok.raw_value);
  }

  if (tok.type == TOK_WORD) {
    if (!tok.raw_value && !tok.parts)
      return (size_t) snprintf(buf, buf_sz, "[TOK_WORD]: (null)\n");

    offset += (size_t) snprintf(temp + offset, sizeof(temp) - offset, "[TOK_WORD]: %s\n", 
                       tok.raw_value ? tok.raw_value : "");

    // -- display parts if they exist --
    if (tok.parts && arrlen(tok.parts) > 0) {
      for (int i = 0; i < arrlen(tok.parts); i++) {
        word_part_t p = tok.parts[i];
        const char *q =
          (p.quote == QUOTE_SINGLE ? "'" :
           p.quote == QUOTE_DOUBLE ? "\"" : "none");

        const char *kind = lexer_part_type_str(p.type);

        offset += (size_t) snprintf(temp + offset, sizeof(temp) - offset,
                           "  - %s(%s, quote=%s)\n",
                           kind, p.value, q);
      }
    }

    if (buf && buf_sz > 0) {
      strncpy(buf, temp, buf_sz - 1);
      buf[buf_sz - 1] = '\0';
    }
    return strlen(temp);
  }

  // -- 3. If none of the above match --
  return (size_t) snprintf(buf, buf_sz, "[UNKNOWN TOKEN]\n");
}

const char *lexer_part_type_str(word_part_type_e type) {
  switch (type) {
    case WORD_LITERAL: return "LITERAL";
    case WORD_VARIABLE: return "VARIABLE";
    case WORD_TILDE: return "TILDE";
    case WORD_GLOB: return "GLOB";
    default: return "UNKNOWN";
  }
}