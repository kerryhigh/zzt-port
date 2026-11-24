#ifndef ZZT_TEST_STDLIB_H
#define ZZT_TEST_STDLIB_H
#include <stddef.h>
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
void qsort(void *base, size_t count, size_t size,
        int (*compare)(const void *, const void *));
#endif
