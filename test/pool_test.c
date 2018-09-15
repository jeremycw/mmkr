#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <pthread/pthread.h>
#include "../src/pool.h"

void* producer(void* arg) {
  pool_t* pool = arg;
  int data[1024];
  for (int i = 0; i < 1024; i++) {
    data[i] = i;
  }
  for (int i = 0; i < 1000; i++) {
    pool_write(pool, data, sizeof(data));
    usleep(100);
  }
  return NULL;
}

void* consumer(void* arg) {
  pool_t* pool = arg;
  int data[1024];
  int reference[1024];
  for (int i = 0; i < 1024; i++) {
    reference[i] = i;
  }
  for (int i = 0; i < 1000; i++) {
    pool_reader_t reader;
    pool_read(pool, &reader, data, sizeof(data));
    assert(memcmp(data, reference, sizeof(data)) == 0);
  }
  return NULL;
}

typedef struct {
  pool_t* pool;
  char* str;
} str_producer_arg_t;

char* str1 = "Starting on version 2, the development of MATHC uses calendar versioning, with a tag YYYY.MM.DD.MICRO for each stable release. If a release breaks backward compatibility, then it is mentioned in the release notes.";
char* str2 = "By default, vectors, quaternions and matrices can be declared as arrays of mint_t, arrays of mfloat_t, or structures.";

void* str_producer(void* arg) {
  str_producer_arg_t* a = arg;
  pool_t* pool = a->pool;
  char* str = a->str;
  for (int i = 0; i < 2000; i++) {
    pool_write(pool, str, strlen(str) + 1);
    usleep(100);
  }
  return NULL;
}

void* str_consumer(void* arg) {
  char str[1024];
  pool_t* pool = arg;
  int count = 0;
  int index = 0;
  pool_reader_t reader = pool_new_reader(pool);
  while (count < 4000) {
    do {
      ssize_t bytes = pool_read(pool, &reader, str + index, 1);
      assert(bytes > 0);
      index++;
    } while (str[index-1]);
    assert(
      strcmp(str, str1) == 0
      || strcmp(str, str2) == 0
    );
    index = 0;
    count++;
  }
  return NULL;
}

void test_pool() {
  {
    pool_t pool;
    pool_new(&pool, 1024);
    int test = 123456789;
    int* ptr;
    int bytes;
    pool_write(&pool, &test, 4);
    pool_swap(&pool, (void*)&ptr, &bytes, 0);
    assert(*ptr == 123456789);
    assert(bytes == 4);
  }

  {
    pool_t pool2;
    pool_new(&pool2, sizeof(int) * 4);
    pool_reader_t reader = pool_new_reader(&pool2);
    int array[6] = {0, 1, 2, 3, 4, 5};
    pool_write(&pool2, array, sizeof(int) * 4);
    int read[6];
    size_t bytes = pool_read(&pool2, &reader, read, sizeof(int) * 4);
    assert(bytes == sizeof(int) * 4);
    for (int i = 0; i < 4; i++) {
      assert(read[i] == i);
    }
    pool_write(&pool2, &array[4], sizeof(int) * 2);
    bytes = pool_read(&pool2, &reader, read, sizeof(int) * 2);
    assert(read[0] == 4);
    assert(read[1] == 5);
    pool_write(&pool2, array, sizeof(int) * 3);
    bytes = pool_read(&pool2, &reader, read, sizeof(int) * 3);
    for (int i = 0; i < 3; i++) {
      assert(read[i] == i);
    }
  }

  {
    pool_t pool;
    pool_new(&pool, 8 * 1024 * 1024);
    pthread_t p, c;
    pthread_create(&p, NULL, producer, &pool);
    pthread_create(&c, NULL, consumer, &pool);
    pthread_join(p, NULL);
    pthread_join(c, NULL);
  }

  {
    pool_t pool;
    pool_new(&pool, 4 * 1024);
    pthread_t p1, p2, c;
    str_producer_arg_t parg1 = { &pool, str1 };
    str_producer_arg_t parg2 = { &pool, str2 };
    pthread_create(&c, NULL, str_consumer, &pool);
    pthread_create(&p1, NULL, str_producer, &parg1);
    pthread_create(&p2, NULL, str_producer, &parg2);
    pthread_join(p1, NULL);
    pthread_join(p2, NULL);
    pthread_join(c, NULL);
  }
}