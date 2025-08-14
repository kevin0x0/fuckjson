#include "strpool.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

constexpr size_t BUFFER_SIZE = 4096;

unsigned char zero_buffer[1];

static void remove_block(struct block *block) {
  struct block *prev = block->prev;
  struct block *next = block->next;
  if (prev)
    prev->next = next;
  if (next)
    next->prev = prev;

  free(block);
}

static inline void load_current_block_info(struct strpool *strpool) {
  struct block *current_block = strpool->current_block;
  strpool->currpos = current_block->curr;
  strpool->remaining_size = current_block->remaining_size;
}

static inline void store_current_block_info(struct strpool *strpool) {
  strpool->current_block->curr = strpool->currpos;
  strpool->current_block->remaining_size = strpool->remaining_size;
}

void strpool_init(struct strpool *strpool) {
  struct block *block = malloc(sizeof(struct block) + BUFFER_SIZE);
  if (unlikely(!block)) {
    fputs("out of memory", stderr);
    exit(1);
  }

  block->remaining_size = BUFFER_SIZE;
  block->end = block->buf + BUFFER_SIZE;
  block->curr = block->buf;
  block->next = NULL;
  block->prev = NULL;

  strpool->current_block = block;

  load_current_block_info(strpool);
}

void strpool_destroy(struct strpool *strpool) {
  struct block *block = strpool->current_block->prev;
  while (block) {
    struct block *prev = block->prev;
    remove_block(block);
    block = prev;
  }

  block = strpool->current_block;
  while (block) {
    struct block *next = block->next;
    remove_block(block);
    block = next;
  }
}

static void next_available_block(struct strpool *strpool, size_t size) {
  while (strpool->current_block->next) {
    struct block *block = strpool->current_block->next;
    if (block->remaining_size >= size) {
      strpool->current_block = block;
      return;
    } else {
      remove_block(block);
    }
  }

  struct block *block = malloc(sizeof(struct block) + size);
  if (unlikely(!block)) {
    fputs("out of memory", stderr);
    exit(1);
  }

  block->remaining_size = size;
  block->end = block->buf + size;
  block->curr = block->buf;
  block->next = NULL;
  block->prev = strpool->current_block;
  strpool->current_block->next = block;

  strpool->current_block = block;
}

static void realloc_current_block(struct strpool *strpool, size_t size) {
  struct block *block = strpool->current_block;
  struct block *prev = block->prev;
  struct block *next = block->next;
  size_t used_size = block->curr - block->buf;

  struct block *new_block = realloc(block, sizeof(struct block) + size);
  if (unlikely(!new_block)) {
    fputs("out of memory", stderr);
    exit(1);
  }

  new_block->remaining_size = size - used_size;
  new_block->end = new_block->buf + size;
  new_block->curr = new_block->buf;
  new_block->prev = prev;
  new_block->next = next;

  if (next)
    next->prev = new_block;
  if (prev)
    prev->next = new_block;

  strpool->current_block = new_block;
}

unsigned char *strpool_alloc_fallback(struct strpool *strpool, size_t size) {
  assert(size != 0);

  store_current_block_info(strpool);

  if (unlikely(strpool->currpos == strpool->current_block->buf)) {
    realloc_current_block(strpool, size);
  } else {
    size_t new_block_size = max(size, BUFFER_SIZE);
    next_available_block(strpool, new_block_size);
  }

  load_current_block_info(strpool);
  return strpool->currpos;
}

unsigned char *strpool_realloc_fallback(struct strpool *strpool,
                                        size_t new_size) {
  assert(new_size != 0);

  store_current_block_info(strpool);

  if (unlikely(strpool->currpos == strpool->current_block->buf)) {
    realloc_current_block(strpool, new_size * 2);
  } else {
    next_available_block(strpool, new_size);

    struct block *prev_block = strpool->current_block->prev;
    memcpy(strpool->current_block->buf, prev_block->curr,
           min(new_size, prev_block->remaining_size));
  }

  load_current_block_info(strpool);
  return strpool->currpos;
}

void strpool_free_fallback(struct strpool *strpool, size_t size) {
  assert(strpool->currpos == strpool->current_block->buf);

  store_current_block_info(strpool);

  strpool->current_block = strpool->current_block->prev;

  assert((size_t)(strpool->current_block->curr - strpool->current_block->buf) >=
         size);

  load_current_block_info(strpool);
  strpool->currpos -= size;
  return;
}
