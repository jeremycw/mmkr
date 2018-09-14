#include <string.h>
#include <stdlib.h>
#include "merge_sort.h"

void merge_sort(void* in, int n, int stride, int (*cmp)(void*,void*)) {
  int size = 1;
  void* array = in;
  int total_bytes = n * stride;
  void* aux = malloc(total_bytes);
  while (size < n) {
    int index = 0;
    int partition_bytes = size * stride;
    for (int base = 0; base < total_bytes; base += partition_bytes * 2) {
      int a1i = 0;
      int a2i = 0;
      void* a1 = &array[base];
      void* a2 = &array[base + partition_bytes];
      int a2size = base + partition_bytes * 2 > total_bytes ?
        total_bytes % partition_bytes : partition_bytes;
      while (a1i != partition_bytes || a2i != a2size) {
        if (
          a2i == a2size //a2 is empty
          || (
            a1i != partition_bytes //a1 not empty
            && cmp(&a1[a1i], &a2[a2i])
          )
        ) {
          memcpy(&aux[index], &a1[a1i], stride);
          a1i += stride;
        } else {
          memcpy(&aux[index], &a2[a2i], stride);
          a2i += stride;
        }
        index += stride;
      }
    }
    size *= 2;
    void* tmp = array;
    array = aux;
    aux = tmp;
  }
  if (array != in) {
    memcpy(in, array, total_bytes);
    free(array);
  } else {
    free(aux);
  }
}
