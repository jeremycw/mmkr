#ifndef POOL_H
#define POOL_H

#include <pthread/pthread.h>
#include <stdlib.h>

#define POOL_NO_FLAGS 0x0
#define POOL_WAIT_FOR_DATA 0x1

typedef struct {
    size_t index;
    int generation;
} gen_index_t;

typedef gen_index_t pool_reader_t;

typedef struct {
    size_t size;
    int done;
} write_t;

typedef struct {
    write_t queue[32];
    int tail;
    int head;
    int size;
    int capacity;
} write_queue_t;

typedef struct {
    void* buffer;
    size_t size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    gen_index_t current;
    gen_index_t safe;
    write_queue_t write_queue;
    int writers;
} pool_t;

void pool_new(pool_t* pool, size_t size);
void pool_write(pool_t* pool, void* src, size_t size);
void pool_swap(pool_t* pool, void** out, int* index, unsigned int flags);
size_t pool_read(pool_t* pool, pool_reader_t* pool_reader, void* dst, size_t size);

#endif