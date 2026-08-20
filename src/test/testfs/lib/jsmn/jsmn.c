#include "jsmn.h"

#ifndef JSMN_PARENT_LINKS
#define JSMN_PARENT_LINKS
#endif

static jsmntok_t *jsmn_alloc_token(jsmn_parser *parser, jsmntok_t *tokens,
                                  const size_t num_tokens) {
  if (parser->toknext >= num_tokens) return NULL;
  jsmntok_t *tok = &tokens[parser->toknext++];
  tok->start = tok->end = -1;
  tok->size = 0;
#ifdef JSMN_PARENT_LINKS
  tok->parent = -1;
#endif
  return tok;
}

static void jsmn_fill_token(jsmntok_t *token, const jsmntype_t type,
                            const int start, const int end) {
  token->type = type;
  token->start = start;
  token->end = end;
  token->size = 0;
}

static int jsmn_parse_primitive(jsmn_parser *parser, const char *js,
                                const size_t len, jsmntok_t *tokens,
                                const size_t num_tokens) {
  int start = (int)parser->pos;
  for (; parser->pos < len; parser->pos++) {
    switch (js[parser->pos]) {
      case '\t': case '\r': case '\n': case ' ':
      case ',': case ']': case '}':
        goto found;
      default:
        break;
    }
  }
found:
  if (tokens == NULL) { parser->pos--; return 0; }
  jsmntok_t *tok = jsmn_alloc_token(parser, tokens, num_tokens);
  if (tok == NULL) { parser->pos = (unsigned int)start; return JSMN_ERROR_NOMEM; }
  jsmn_fill_token(tok, JSMN_PRIMITIVE, start, (int)parser->pos);
#ifdef JSMN_PARENT_LINKS
  tok->parent = parser->toksuper;
#endif
  parser->pos--;
  return 0;
}

static int jsmn_parse_string(jsmn_parser *parser, const char *js,
                             const size_t len, jsmntok_t *tokens,
                             const size_t num_tokens) {
  int start = (int)parser->pos;
  parser->pos++;

  for (; parser->pos < len; parser->pos++) {
    char c = js[parser->pos];

    if (c == '\"') {
      if (tokens == NULL) return 0;
      jsmntok_t *tok = jsmn_alloc_token(parser, tokens, num_tokens);
      if (tok == NULL) { parser->pos = (unsigned int)start; return JSMN_ERROR_NOMEM; }
      jsmn_fill_token(tok, JSMN_STRING, start + 1, (int)parser->pos);
#ifdef JSMN_PARENT_LINKS
      tok->parent = parser->toksuper;
#endif
      return 0;
    }

    if (c == '\\' && parser->pos + 1 < len) {
      parser->pos++;
      continue;
    }
  }
  parser->pos = (unsigned int)start;
  return JSMN_ERROR_PART;
}

void jsmn_init(jsmn_parser *parser) {
  parser->pos = 0;
  parser->toknext = 0;
  parser->toksuper = -1;
}

int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
               jsmntok_t *tokens, unsigned int num_tokens) {
  int r;
  int i;
  jsmntok_t *token;

  for (; parser->pos < len; parser->pos++) {
    char c = js[parser->pos];
    jsmntype_t type;

    switch (c) {
      case '{': case '[':
        if (tokens == NULL) break;
        token = jsmn_alloc_token(parser, tokens, num_tokens);
        if (token == NULL) return JSMN_ERROR_NOMEM;
        type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
        jsmn_fill_token(token, type, (int)parser->pos, -1);
#ifdef JSMN_PARENT_LINKS
        token->parent = parser->toksuper;
#endif
        if (parser->toksuper != -1) tokens[parser->toksuper].size++;
        parser->toksuper = (int)(parser->toknext - 1);
        break;

      case '}': case ']':
        if (tokens == NULL) break;
        type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
        for (i = (int)parser->toknext - 1; i >= 0; i--) {
          token = &tokens[i];
          if (token->start != -1 && token->end == -1) {
            if (token->type != type) return JSMN_ERROR_INVAL;
            token->end = (int)parser->pos + 1;
            parser->toksuper =
#ifdef JSMN_PARENT_LINKS
              token->parent
#else
              -1
#endif
            ;
            break;
          }
        }
        if (i == -1) return JSMN_ERROR_INVAL;
        break;

      case '\"':
        r = jsmn_parse_string(parser, js, len, tokens, num_tokens);
        if (r < 0) return r;
        if (parser->toksuper != -1 && tokens != NULL) tokens[parser->toksuper].size++;
        break;

      case '\t': case '\r': case '\n': case ' ':
      case ':': case ',':
        break;

      default:
        r = jsmn_parse_primitive(parser, js, len, tokens, num_tokens);
        if (r < 0) return r;
        if (parser->toksuper != -1 && tokens != NULL) tokens[parser->toksuper].size++;
        break;
    }
  }

  if (tokens != NULL) {
    for (i = (int)parser->toknext - 1; i >= 0; i--) {
      if (tokens[i].start != -1 && tokens[i].end == -1) return JSMN_ERROR_PART;
    }
  }

  return (int)parser->toknext;
}
