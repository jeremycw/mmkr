#include <assert.h>
#include <stdlib.h>
#include "../src/pool.h"

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
        int array[6] = {0, 1, 2, 3, 4, 5};
        pool_write(&pool2, array, sizeof(int) * 4);
        int read[6];
        pool_reader_t reader = {0, 0};
        size_t bytes = pool_read(&pool2, &reader, read, sizeof(int) * 4);
        assert(bytes == sizeof(int) * 4);
        for (int i = 0; i < 4; i++) {
            assert(read[i] == i);
        }
        pool_write(&pool2, &array[4], sizeof(int) * 2);
        bytes = pool_read(&pool2, &reader, read, sizeof(int) * 2);
        assert(read[0] == 4);
        assert(read[1] == 5);
        pool_write(&pool2, array, sizeof(int) * 6);
        bytes = pool_read(&pool2, &reader, read, sizeof(int) * 6);
        for (int i = 0; i < 4; i++) {
            assert(read[i] == i);
        }
    }
}