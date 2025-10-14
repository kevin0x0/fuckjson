#include "parser.h"
#include "match.h"
#include "strpool.h"
#include "utils.h"

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

[[noreturn]] static void error(struct parser *parser, const char *fmt, ...) {
  va_list ap;

  fprintf(stderr, "error in offset %ld: ", ftell(parser->file));
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fputc('\n', stderr);
  exit(1);
}

#define lex_return(tok)                                                        \
  do {                                                                         \
    parser->kind = tok;                                                        \
    return;                                                                    \
  } while (0)

static inline bool is_digit(int ch) {
  return ch >= '0' && ch <= '9';
}

static inline bool is_alpha(int ch) {
  return (ch | 0x20) >= 'a' && (ch | 0x20) <= 'z';
}

static inline bool is_cntrl(int ch) {
  return ch < 32 || ch == 127;
}

#define PARSE_NUMBER(on_get)                                                   \
  do {                                                                         \
    do {                                                                       \
      on_get;                                                                  \
    } while (is_digit(ch = fgetc(parser->file)));                              \
                                                                               \
    if (ch == '.') {                                                           \
      do {                                                                     \
        on_get;                                                                \
      } while (is_digit(ch = fgetc(parser->file)));                            \
    }                                                                          \
                                                                               \
    if (ch == 'e' || ch == 'E') {                                              \
      on_get;                                                                  \
      ch = fgetc(parser->file);                                                \
      if (ch == '+' || ch == '-' || is_digit(ch)) {                            \
        do {                                                                   \
          on_get;                                                              \
        } while (is_digit(ch = fgetc(parser->file)));                          \
      }                                                                        \
    }                                                                          \
                                                                               \
    if (likely(ch != EOF))                                                     \
      ungetc(ch, parser->file);                                                \
  } while (0);

static inline void parse_number(struct parser *parser, int ch) {
  size_t bufsize = 32;
  unsigned char *buffer = strpool_alloc(parser->strpool, bufsize);
  size_t currpos = 0;

  PARSE_NUMBER({
    if (unlikely(currpos >= bufsize)) {
      bufsize *= 2;
      buffer = strpool_realloc(parser->strpool, bufsize);
    }
    buffer[currpos++] = ch;
  });

  parser->attr.number = buffer;
  parser->length = currpos;
  lex_return(TK_NUMBER);
}

static wint_t escaped_char(struct parser *parser) {
  int ch = fgetc(parser->file);
  if (unlikely(ch == EOF))
    error(parser, "unterminated string");

  switch (ch) {
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'v':
      return '\v';
    case 't':
      return '\t';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case '0':
      return '\0';
    case 'u': {
      char digits[5];
      for (size_t i = 0; i < ARRAY_SIZE(digits) - 1; ++i) {
        if (unlikely((ch = fgetc(parser->file)) == EOF))
          error(parser, "unterminated string");
        digits[i] = ch;
      }
      digits[ARRAY_SIZE(digits) - 1] = '\0';
      return (wint_t)strtoul(digits, NULL, 16);
    }
    case '\\':
      return '\\';
    default:
      return ch;
  }
}

static size_t encode_utf8_len(unsigned long ch) {
  if (ch <= 0x7F) {
    return 1;
  } else if (ch <= 0x7FF) {
    return 2;
  } else if (ch <= 0xFFFF) {
    return 3;
  } else if (ch <= 0x10FFFF) {
    return 4;
  }
  return 0;
}

static void encode_utf8(unsigned long ch, unsigned char *buffer) {
    if (ch <= 0x7F) {
        buffer[0] = (char)ch;
    } else if (ch <= 0x7FF) {
        buffer[0] = (char)(0xC0 | (ch >> 6));
        buffer[1] = (char)(0x80 | (ch & 0x3F));
    } else if (ch <= 0xFFFF) {
        buffer[0] = (char)(0xE0 | (ch >> 12));
        buffer[1] = (char)(0x80 | ((ch >> 6) & 0x3F));
        buffer[2] = (char)(0x80 | (ch & 0x3F));
    } else if (ch <= 0x10FFFF) {
        buffer[0] = (char)(0xF0 | (ch >> 18));
        buffer[1] = (char)(0x80 | ((ch >> 12) & 0x3F));
        buffer[2] = (char)(0x80 | ((ch >> 6) & 0x3F));
        buffer[3] = (char)(0x80 | (ch & 0x3F));
    }
}

static void parse_string(struct parser *parser) {
  size_t bufsize = 256;
  unsigned char *buffer = strpool_alloc(parser->strpool, bufsize);
  size_t currpos = 0;


  while (true) {
    int ch = fgetc(parser->file);
    if (unlikely(ch == EOF))
      error(parser, "unterminated string");

    if (ch == '"')
      break;

    if (unlikely(currpos == bufsize)) {
      bufsize *= 2;
      buffer = strpool_realloc(parser->strpool, bufsize);
    }

    if (unlikely(ch == '\\')) {
      unsigned long ch = escaped_char(parser);
      size_t len = encode_utf8_len(ch);
      if (unlikely(len + currpos > bufsize)) {
        bufsize += len;
        buffer = strpool_realloc(parser->strpool, bufsize);
      }
      encode_utf8(ch, buffer + currpos);
      currpos += len;
    } else {
      buffer[currpos++] = ch;
    }
  }

  parser->attr.string = buffer;
  parser->length = currpos;

  lex_return(TK_STRING);
}

static void next(struct parser *parser) {
retry:
  int ch = fgetc(parser->file);

  if (unlikely(ch == EOF))
    lex_return(TK_EOF);

  if (unlikely(ch < 0))
    unreachable();

  if (unlikely(ch > 255))
    unreachable();

  switch (ch) {
    case ':':
      lex_return(TK_COLON);
    case ',':
      lex_return(TK_COMMA);
    case '{':
      lex_return(TK_LBRACE);
    case '}':
      lex_return(TK_RBRACE);
    case '[':
      lex_return(TK_LBRACKET);
    case ']':
      lex_return(TK_RBRACKET);
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      parse_number(parser, ch);
      return;
    case 'f':
    case 't': {
      parser->attr.boolean = ch == 'f' ? false : true;

      while (is_alpha(ch = fgetc(parser->file)))
        continue;

      if (unlikely(ch != EOF))
        ungetc(ch, parser->file);
      
      lex_return(TK_BOOL);
    }
    case 'n': {
      while (is_alpha(ch = fgetc(parser->file)))
        continue;

      if (unlikely(ch != EOF))
        ungetc(ch, parser->file);

      lex_return(TK_NULL);
    }
    case '"':
      parse_string(parser);
      return;
    case ' ':
    case '\t':
    case '\n':
      goto retry;
  }
}

static const char *token_desc[TK_NKIND] = {
  [TK_NUMBER] = "number",
  [TK_BOOL] = "bool",
  [TK_STRING] = "string",
  [TK_NULL] = "null",
  [TK_LBRACE] = "'{'",
  [TK_RBRACE] = "'}'",
  [TK_LBRACKET] = "'['",
  [TK_RBRACKET] = "']'",
  [TK_EOF] = "EOF",
  [TK_COMMA] = "','",
  [TK_COLON] = "':'",
};

static inline void expect(struct parser *parser, enum tokenkind kind) {
  if (unlikely(parser->kind != kind))
    error(parser, "expect %s, got %s", token_desc[kind], token_desc[parser->kind]);
}

static inline void lex_match(struct parser *parser, enum tokenkind expected) {
  if (unlikely(parser->kind != expected))
    error(parser, "expect %s, got %s", token_desc[expected],
          token_desc[parser->kind]);

  next(parser);
}

static void skip_value(struct parser *parser);

static void skip_object(struct parser *parser) {
  assert(parser->kind == TK_LBRACE);

  next(parser);
  while (parser->kind != TK_RBRACE) {
    lex_match(parser, TK_STRING);
    lex_match(parser, TK_COLON);
    skip_value(parser);
    if (parser->kind == TK_COMMA)
      next(parser);
  }

  next(parser);
}

static void skip_array(struct parser *parser) {
  assert(parser->kind == TK_LBRACKET);

  next(parser);
  while (parser->kind != TK_RBRACKET) {
    skip_value(parser);
    if (parser->kind == TK_COMMA)
      next(parser);
  }

  next(parser);
}

static void skip_value(struct parser *parser) {
  switch (parser->kind) {
    case TK_LBRACE:
      skip_object(parser);
      return;
    case TK_LBRACKET:
      skip_array(parser);
      return;
    case TK_BOOL:
    case TK_NULL:
    case TK_NUMBER:
    case TK_STRING:
      next(parser);
      return;
    default:
      error(parser, "unexpected %s", token_desc[parser->kind]);
  }
}

static inline void print_token(struct parser *parser) {
  switch (parser->kind) {
    case TK_COMMA:
      fputc(',', stdout);
      break;
    case TK_COLON:
      fputc(':', stdout);
      break;
    case TK_LBRACE:
      fputc('{', stdout);
      break;
    case TK_RBRACE:
      fputc('}', stdout);
      break;
    case TK_LBRACKET:
      fputc('[', stdout);
      break;
    case TK_RBRACKET:
      fputc(']', stdout);
      break;
    case TK_BOOL:
      fputs(parser->attr.boolean ? "true" : "false", stdout);
      break;
    case TK_NULL:
      fputs("null", stdout);
      break;
    case TK_NUMBER:
      fwrite(parser->attr.number, 1, parser->length, stdout);
      break;
    case TK_STRING:
      fwrite(parser->attr.string, 1, parser->length, stdout);
      break;
    case TK_EOF:
      error(parser, "unexpected %s", token_desc[parser->kind]);
    default:
      unreachable();
  }
}

static inline void print_and_match(struct parser *parser, enum tokenkind expected) {
  print_token(parser);
  lex_match(parser, expected);
}

static inline void print_and_next(struct parser *parser) {
  print_token(parser);
  next(parser);
}

static void print_value(struct parser *parser);

static void print_object(struct parser *parser) {
  assert(parser->kind == TK_LBRACE);

  print_and_next(parser);
  while (parser->kind != TK_RBRACE) {
    print_and_match(parser, TK_STRING);
    print_and_match(parser, TK_COLON);
    print_value(parser);
    if (parser->kind == TK_COMMA)
      print_and_next(parser);
  }

  print_and_next(parser);
}

static void print_array(struct parser *parser) {
  assert(parser->kind == TK_LBRACKET);

  print_and_next(parser);
  while (parser->kind != TK_RBRACKET) {
    print_value(parser);
    if (parser->kind == TK_COMMA)
      print_and_next(parser);
  }

  print_and_next(parser);
}

static inline char to_hex_digit(char ch) {
  if (ch <= 9)
    return '0' + ch;

  return 'A' + (ch - 10);
}

static void print_escape(unsigned char ch) {
  const char *s;
  switch (ch) {
    case '\r':
      s = "\\r";
      break;
    case '\f':
      s = "\\f";
      break;
    case '\n':
      s = "\\n";
      break;
    case '\t':
      s = "\\t";
      break;
    case '\b':
      s = "\\b";
      break;
    case '"':
      s = "\\\"";
      break;
    case '\\':
      s = "\\\\";
      break;
    default: {
      fputs("\\u00", stdout);
      fputc(to_hex_digit(ch >> 4), stdout);
      fputc(to_hex_digit(ch & 15), stdout);
      return;
    }
  }
  fputs(s, stdout);
}

static void print_string(struct parser *parser) {
  unsigned char *s = parser->attr.string;
  unsigned char *end = s + parser->length;
  fputc('"', stdout);

  while (true) {
    unsigned char *curr = s;
    while (likely(!(curr == end || is_cntrl(*curr) || *curr == '"' || *curr == '\\')))
      ++curr;

    fwrite(s, 1, curr - s, stdout);

    if (unlikely(curr == end))
      break;

    print_escape(*curr);
    s = curr + 1;
  }

  fputc('"', stdout);
  next(parser);
}

static void print_value(struct parser *parser) {
  switch (parser->kind) {
    case TK_LBRACE:
      print_object(parser);
      return;
    case TK_LBRACKET:
      print_array(parser);
      return;
    case TK_STRING:
      print_string(parser);
      return;
    case TK_BOOL:
    case TK_NULL:
    case TK_NUMBER:
      print_and_next(parser);
      return;
    default:
      error(parser, "unexpected %s", token_desc[parser->kind]);
  }
}

#define FOR_EACH_KEY(on_key)                                                   \
  do {                                                                         \
    assert(parser->kind == TK_LBRACE);                                         \
    next(parser);                                                              \
    while (parser->kind != TK_RBRACE) {                                        \
      expect(parser, TK_STRING);                                               \
      on_key;                                                                  \
      if (parser->kind == TK_COMMA)                                            \
        next(parser);                                                          \
    }                                                                          \
    next(parser);                                                              \
  } while (0)

#define FOR_EACH_ELEMENT(on_element)                                           \
  do {                                                                         \
    size_t index = 0;                                                          \
    assert(parser->kind == TK_LBRACKET);                                       \
    next(parser);                                                              \
    while (parser->kind != TK_RBRACKET) {                                      \
      on_element;                                                              \
      ++index;                                                                 \
      if (parser->kind == TK_COMMA)                                            \
        next(parser);                                                          \
    }                                                                          \
    next(parser);                                                              \
  } while (0)

static void do_match(struct parser * parser, struct match *match);

static void match_on_object(struct parser *parser, struct match *match) {
  assert(parser->kind == TK_LBRACE);

  struct selector *end = match->selectors + match->nselector;
  for (struct selector *p = match->selectors; p != end; ++p) {
    if (can_match_key(p->type))
      goto start;
  }

  skip_value(parser);
  return;

start:
  FOR_EACH_KEY({
    for (struct selector *p = match->selectors; p != end; ++p) {
      if (p->type != MATCH_ALL_KEY &&
          (p->type != MATCH_KEY || parser->length != p->expected_keylen ||
           memcmp(parser->attr.string, p->expected.key, parser->length)) != 0) {
        continue;
      }
      strpool_commit(parser->strpool, parser->length);
      p->matched.key = parser->attr.string;
      p->matched_keylen = parser->length;
      next(parser);
      lex_match(parser, TK_COLON);
      do_match(parser, p->submatch);
      strpool_free(parser->strpool, p->matched_keylen);
      goto next_loop;
    }
    next(parser);
    lex_match(parser, TK_COLON);
    skip_value(parser);
  next_loop:
  });
}

static void match_on_array(struct parser *parser, struct match *match) {
  assert(parser->kind == TK_LBRACKET);

  struct selector *end = match->selectors + match->nselector;
  for (struct selector *p = match->selectors; p != end; ++p) {
    if (can_match_index(p->type))
      goto start;
  }

  skip_value(parser);
  return;

start:
  FOR_EACH_ELEMENT({
    for (struct selector *p = match->selectors; p != end; ++p) {
      if (p->type != MATCH_ALL_INDEX &&
          (p->type != MATCH_INDEX || index != p->expected.index)) {
        continue;
      }

      p->matched.index = index;

      do_match(parser, p->submatch);

      goto next_loop;
    }
    skip_value(parser);
  next_loop:
  });
}

static void do_match(struct parser *parser, struct match *match) {
  if (!match) {
    if ((parser->print_option & PRINT_RAW) && parser->kind == TK_STRING) {
      fwrite(parser->attr.string, 1, parser->length, stdout);
      next(parser);
    } else {
      print_value(parser);
    }
    if (parser->print_option & PRINT_NULL_SEP) {
      fputc('\0', stdout);
    } else {
      fputs(parser->delimiter, stdout);
    }

    if (parser->print_option & PRINT_FLUSH_STDOUT)
      fflush(stdout);

    return;
  }

  switch (parser->kind) {
    case TK_LBRACE: {
      match_on_object(parser, match);
      return;
    }
    case TK_LBRACKET: {
      match_on_array(parser, match);
      return;
    }
    default: {
      skip_value(parser);
      return;
    }
  }
}

void start_matching(struct parser *parser, struct match *match) {
  next(parser);
  do_match(parser, match);
}

void start_stream_matching(struct parser *parser, struct match *match) {
  next(parser);
  while (parser->kind != TK_EOF)
    do_match(parser, match);
}
