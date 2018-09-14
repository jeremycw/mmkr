#ifndef MERGE_SORT_H
#define MERGE_SORT_H

void merge_sort(void* in, int n, int stride, int (*cmp)(void*,void*));

#endif