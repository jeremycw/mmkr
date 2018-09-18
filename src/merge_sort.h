#ifndef MERGE_SORT_H
#define MERGE_SORT_H

void merge_sort(void* in, int n, int stride, int (*cmp)(void*,void*));
void merge(void* a, void* b, void* out, int an, int bn, int stride, int* index, int (*cmp)(void*,void*));

#endif