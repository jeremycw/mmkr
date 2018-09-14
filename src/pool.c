#include <stdlib.h>
#include <pthread/pthread.h>
#include <string.h>
#include <assert.h>

#include "pool.h"

void pool_new(pool_t* pool, ssize_t capacity) {
  pool->current.generation = 0;
  pool->current.index = 0;
  pool->safe.generation = 0;
  pool->safe.index = 0;
  pool->writers = 0;
  pool->capacity = capacity;
  pool->buffer = malloc(pool->capacity);
  pool->write_queue.head = 0;
  pool->write_queue.tail = 0;
  pool->write_queue.size = 0;
  pool->write_queue.capacity = 32;
  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->cond, NULL);
}

write_t* write_queue_enqueue(write_queue_t* write_queue, ssize_t bytes) {
  assert(write_queue->tail < write_queue->capacity);
  write_queue->queue[write_queue->tail].done = 0;
  write_queue->queue[write_queue->head].bytes = bytes;
  write_t* w = &write_queue->queue[write_queue->tail];
  write_queue->tail = (write_queue->tail + 1) % write_queue->capacity;
  return w;
}

gen_index_t increment_index(
  gen_index_t gi,
  ssize_t capacity,
  ssize_t increment
) {
  ssize_t initial = gi.index;
  gi.index = (gi.index + increment) % capacity;
  if (gi.index < initial || increment == capacity) gi.generation++;
  return gi;
}

int cmp_index(gen_index_t a, gen_index_t b) {
  if (
    a.generation > b.generation
    || (a.index > b.index && a.generation == b.generation)
  ) {
    return 1;
  } else if (a.index == b.index && a.generation == b.generation) {
    return 0;
  } else {
    return -1;
  }
}

int write_queue_process(pool_t* pool) {
  gen_index_t initial = pool->safe;
  write_queue_t* write_queue = &pool->write_queue;
  while (
    write_queue->queue[write_queue->head].done
    && write_queue->head < write_queue->tail
  ) {
    ssize_t bytes = write_queue->queue[write_queue->head].bytes;
    pool->safe = increment_index(pool->safe, pool->capacity, bytes);
    write_queue->head++;
  }
  if (write_queue->head == write_queue->tail) {
    write_queue->head = 0;
    write_queue->tail = 0;
  }
  return cmp_index(initial, pool->safe);
}

void pool_write(pool_t* pool, void* src, ssize_t bytes) {
  pthread_mutex_lock(&pool->mutex);
  void* addr = &pool->buffer[pool->current.index];
  gen_index_t initial = pool->current;
  pool->current = increment_index(pool->current, pool->capacity, bytes);
  pool->writers++;
  write_t* write = write_queue_enqueue(&pool->write_queue, bytes);
  pthread_mutex_unlock(&pool->mutex);
  if (pool->current.index < initial.index) {
    //split write
    ssize_t first_bytes = pool->capacity - initial.index;
    memcpy(addr, src, first_bytes);
    memcpy(pool->buffer, src + first_bytes, bytes - first_bytes);
  } else {
    memcpy(addr, src, bytes);
  }
  pthread_mutex_lock(&pool->mutex);
  pool->writers--;
  write->done = 1;
  write_queue_process(pool);
  if (pool->writers == 0) pthread_cond_signal(&pool->cond);
  pthread_mutex_unlock(&pool->mutex);
}

ssize_t copy(
  pool_t* pool,
  pool_reader_t* reader,
  void* dst,
  ssize_t safe_bytes,
  ssize_t desired_bytes
) {
  ssize_t bytes = safe_bytes > desired_bytes ? desired_bytes : safe_bytes;
  memcpy(dst, &pool->buffer[reader->index], bytes);
  *reader = increment_index(*reader, pool->capacity, bytes);
  return bytes;
}

ssize_t pool_read(
  pool_t* pool,
  pool_reader_t* pool_reader,
  void* dst,
  ssize_t bytes
) {
  //reader too far behind writes, lost data
  if (
    pool_reader->index < pool->current.index
    && pool_reader->generation < pool->current.generation
  ) {
    return -1;
  }
  pthread_mutex_lock(&pool->mutex);
  if (cmp_index(*pool_reader, pool->safe) != -1) {
    pthread_cond_wait(&pool->cond, &pool->mutex);
  }
  gen_index_t safe = pool->safe;
  pthread_mutex_unlock(&pool->mutex);
  if (safe.index > pool_reader->index) {
    ssize_t diff = safe.index - pool_reader->index;
    return copy(pool, pool_reader, dst, diff, bytes);
  } else {
    ssize_t first_bytes = pool->capacity - pool_reader->index;
    ssize_t actual_bytes = 0;
    if (first_bytes > 0) {
      actual_bytes = copy(pool, pool_reader, dst, first_bytes, bytes);
      if (actual_bytes <= bytes) return actual_bytes;
    }
    ssize_t second_bytes = safe.index;
    actual_bytes +=
      copy(pool, pool_reader, dst + first_bytes, second_bytes, bytes);
    return actual_bytes;
  }
}

void pool_swap(pool_t* pool, void** out, int* bytes, unsigned int flags) {
  pthread_mutex_lock(&pool->mutex);
  if (
    pool->writers > 0
    || (flags & POOL_WAIT_FOR_DATA && pool->current.index == 0)
  ) {
    pthread_cond_wait(&pool->cond, &pool->mutex);
  }
  *bytes = pool->current.index;
  *out = pool->buffer;
  pool->buffer = malloc(pool->capacity);
  pool->current.index = 0;
  pthread_mutex_unlock(&pool->mutex);
}