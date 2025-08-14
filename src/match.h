#ifndef _MATCH_H
#define _MATCH_H

#include <stddef.h>

enum selector_type: unsigned char {
  MATCH_ALL_INDEX,
  MATCH_INDEX,
  MATCH_ALL_KEY,
  MATCH_KEY,
};

static inline bool can_match_key(enum selector_type type) {
  return type >= MATCH_ALL_KEY;
}

static inline bool can_match_index(enum selector_type type) {
  return type <= MATCH_INDEX;
}

struct selector {
  struct match *submatch;
  union {
    unsigned char *key;
    size_t index;
  } expected;
  union {
    unsigned char *key;
    size_t index;
  } matched;
  unsigned int expected_keylen;
  unsigned int matched_keylen;
  enum selector_type type;
};

struct match {
  size_t nselector;
  struct selector selectors[];
};

struct match *match_parse(const char *match);
void match_delete(struct match *match);

#endif
