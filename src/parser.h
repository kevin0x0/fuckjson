#ifndef _PARSER_H
#define _PARSER_H

#include "match.h"

#include <assert.h>
#include <stdio.h>

union tokenattr {
  unsigned char *string;
  unsigned char *number;
  bool boolean;
};

enum tokenkind: unsigned char {
  TK_NUMBER,
  TK_STRING,
  TK_BOOL,
  TK_NULL,
  TK_COLON,
  TK_COMMA,
  TK_LBRACE,
  TK_RBRACE,
  TK_LBRACKET,
  TK_RBRACKET,
  TK_EOF,
  TK_NKIND,
};

enum print_option: unsigned char {
  PRINT_NONE = 0,
  PRINT_RAW = 1,
  PRINT_NULL_SEP = 2,
  PRINT_FLUSH_STDOUT = 4,
};

struct parser {
  FILE *file;
  union tokenattr attr;
  unsigned int length;
  enum tokenkind kind;
  enum print_option print_option;
  struct strpool *strpool;
  const char *delimiter;
};

void start_matching(struct parser *parser, struct match *match);
void start_stream_matching(struct parser *parser, struct match *match);

#endif
