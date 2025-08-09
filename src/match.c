#include "match.h"
#include "utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct parse_state {
  const char *command;
  const char *current;
};

struct string {
  unsigned char *buf;
  size_t length;
};

[[noreturn]] static void error(struct parse_state *state, const char *fmt,
                               ...) {
  va_list ap;

  fprintf(stderr, "invalid command: %s\n", state->command);
  fprintf(stderr, "               | %.*s^\n",
          (int)(state->current - state->command), state->command);
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fputc('\n', stderr);
  exit(1);
}

static unsigned char escaped_char(struct parse_state *state) {
  unsigned char ch = *state->current++;
  switch (ch) {
    case '\0':
      error(state, "unexpected '\\0'");
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
    case 'x': {
      ++state->current;
      if (state->current[0] == '\0' || state->current[1] == '\0')
        error(state, "unexpected '\\0'");
      char digits[3];
      digits[0] = *state->current++;
      digits[1] = *state->current++;
      digits[2] = '\0';
      return (unsigned char)strtoul(digits, NULL, 16);
    }
    case '\\':
      return '\\';
    default:
      return ch;
  }
}

static size_t calc_strlen(struct parse_state *state, char endchars[]) {
  size_t string_length = 0;

  while (true) {
    if (strchr(endchars, *state->current)) {
      break;
    }

    ++string_length;

    if (likely(*state->current != '\\')) {
      ++state->current;
    } else {
      if (state->current[1] == 'x') {
        if (unlikely(state->current[2] == '\0' || state->current[3] == '\0'))
          error(state, "unterminated escape sequence");
        state->current += 4;
      } else {
        state->current += 2;
      }
    }
  }

  return string_length;
}

static void copy_string_to_buffer(struct parse_state *state, unsigned char *p,
                                  unsigned char *end) {
  while (p != end) {
    if (*state->current == '\\') {
      ++state->current;
      *p++ = escaped_char(state);
    } else {
      *p++ = *state->current++;
    }
  }
}

static struct string parse_string(struct parse_state *state) {
  struct string ret;
  if (*state->current == '"') {
    const char *strbegin = ++state->current;
    size_t string_length = calc_strlen(state, "\"");

    unsigned char *buf = malloc(string_length);

    if (unlikely(!buf))
      error(state, "out of memory");

    state->current = strbegin;
    copy_string_to_buffer(state, buf, buf + string_length);

    if (*state->current != '"')
      error(state, "unterminated string");

    /* skip '"' */
    ++state->current;

    ret.buf = buf;
    ret.length = string_length;
  } else {
    const char *strbegin = state->current;
    size_t string_length = calc_strlen(state, "{}[.,");

    unsigned char *buf = malloc(string_length);

    if (unlikely(!buf))
      error(state, "out of memory");

    state->current = strbegin;
    copy_string_to_buffer(state, buf, buf + string_length);

    ret.buf = buf;
    ret.length = string_length;
  }

  return ret;
}

static struct match *parse_primary(struct parse_state *state);

static struct selector parse_selector(struct parse_state *state) {
  struct selector selector;

  switch (*state->current++) {
    case '.': {
      if (*state->current == '*') {
        ++state->current;
        selector.type = MATCH_ALL_KEY;
      } else {
        struct string key = parse_string(state);
        selector.type = MATCH_KEY;
        selector.expected.key = key.buf;
        selector.expected_keylen = key.length;
      }
      break;
    }
    case '[': {
      if (*state->current == '*') {
        if (unlikely(*++state->current != ']'))
          error(state, "expected ']'");

        ++state->current;
        selector.type = MATCH_ALL_INDEX;
      } else {
        char *end;
        size_t index = strtoull((char *)state->current, (char **)&end, 0);

        if (unlikely(end == state->current))
          error(state, "expected a integer");

        state->current = end;

        if (unlikely(*state->current++ != ']'))
          error(state, "expected ']'");

        selector.type = MATCH_INDEX;
        selector.expected.index = index;
      }
      break;
    }
    default:
      unreachable();
  }

  selector.submatch = parse_primary(state);
  return selector;
}

static struct match *realloc_match(struct parse_state *state, struct match *ptr,
                                   size_t nselector) {
  struct match *match =
      realloc(ptr, sizeof(struct match) + sizeof(struct selector) * nselector);
  if (unlikely(!match))
    error(state, "out of memory");

  return match;
}

static struct match *parse_singlematch(struct parse_state *state) {
  struct match *match = realloc_match(state, NULL, 1);
  struct selector selector = parse_selector(state);
  match->nselector = 1;
  match->selectors[0] = selector;

  return match;
}

static struct match *parse_multimatch(struct parse_state *state) {
  /* skip '{' */
  ++state->current;

  size_t cap = 4;
  struct match *match =
      malloc(sizeof(struct match) + sizeof(struct selector) * cap);
  if (unlikely(!match))
    error(state, "out of memory");

  match->nselector = 0;

  while (*state->current != '\0') {
    if (*state->current == '}') {
      ++state->current;
      match = realloc_match(state, match, match->nselector);
      return match;
    }

    if (*state->current == ',')
      ++state->current;

    struct selector selector = parse_selector(state);

    if (unlikely(match->nselector >= cap)) {
      match = realloc_match(state, match, 2 * cap);
      cap = 2 * cap;
    }

    match->selectors[match->nselector++] = selector;
  }

  error(state, "unclosed '{'");
}

static struct match *parse_primary(struct parse_state *state) {
  switch (*state->current) {
    case '[':
    case '.': {
      return parse_singlematch(state);
    }
    case '{': {
      return parse_multimatch(state);
    }
    default:
      return NULL;
  }
}

struct match *match_parse(const char *command) {
  struct parse_state state = {
    .command = command,
    .current = command,
  };

  struct match *match = parse_primary(&state);
  if (unlikely(*state.current != '\0'))
    error(&state, "unexpected character");

  return match;
}

void match_delete(struct match *match) {
  if (!match)
    return;

  for (size_t i = 0; i < match->nselector; ++i) {
    match_delete(match->selectors[i].submatch);
    if (match->selectors[i].type == MATCH_KEY)
      free(match->selectors[i].expected.key);
  }
  free(match);
}
