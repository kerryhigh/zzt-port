#ifndef ZZT_TEST_STRING_H
#define ZZT_TEST_STRING_H
#include <stddef.h>
void *memcpy(void *dest, const void *src, size_t count);
void *memset(void *dest, int value, size_t count);
void *memmove(void *dest, const void *src, size_t count);
int memcmp(const void *left, const void *right, size_t count);
int strcmp(const char *left, const char *right);
int strncmp(const char *left, const char *right, size_t count);
int strcasecmp(const char *left, const char *right);
char *strncpy(char *dest, const char *src, size_t count);
char *strncat(char *dest, const char *src, size_t count);
size_t strlen(const char *str);
char *strchr(const char *str, int value);
size_t strnlen(const char *str, size_t maxlen);
#endif
