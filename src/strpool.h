#ifndef _STRPOOL_H
#define _STRPOOL_H

#include "utils.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#if defined (__GNUC__) || defined (__clang__)
constexpr size_t FUNDAMENTAL_ALIGNS = __builtin_ctz(alignof(max_align_t)) + 1;
#else
constexpr size_t FUNDAMENTAL_ALIGNS = 5;
#endif

struct block {
  struct block *next;
  struct block *prev;
  unsigned char *end;
  unsigned char *curr;
  size_t remaining_size;
  unsigned char buf[];
};

struct strpool {
  unsigned char *currpos;
  size_t remaining_size;
  struct block *current_block;
};

void strpool_init(struct strpool *strpool);
void strpool_destroy(struct strpool *strpool);

unsigned char *strpool_alloc_fallback(struct strpool *strpool, size_t size);
unsigned char *strpool_realloc_fallback(struct strpool *strpool,
                                        size_t new_size);
void strpool_free_fallback(struct strpool *strpool, size_t size);

extern unsigned char zero_buffer[1];
static inline unsigned char *strpool_alloc(struct strpool *strpool,
                                           size_t size) {
  if (unlikely(size == 0))
    return zero_buffer;

  if (unlikely(size > strpool->remaining_size))
    return strpool_alloc_fallback(strpool, size);

  return strpool->currpos;
}

static inline void strpool_commit(struct strpool *strpool, size_t size) {
  assert(strpool->remaining_size >= size);

  strpool->currpos += size;
  strpool->remaining_size -= size;
}

static inline void strpool_free(struct strpool *strpool, size_t size) {
  if (strpool->currpos == strpool->current_block->buf) {
    strpool_free_fallback(strpool, size);
    return;
  }

  assert((size_t)(strpool->currpos - strpool->current_block->buf) >= size);

  strpool->currpos -= size;
  strpool->remaining_size += size;
}

static inline unsigned char *
strpool_realloc(struct strpool *strpool, size_t new_size) {
  if (unlikely(new_size == 0))
    return zero_buffer;

  if (unlikely(new_size > strpool->remaining_size))
    return strpool_realloc_fallback(strpool, new_size);

  return strpool->currpos;
}

#endif
