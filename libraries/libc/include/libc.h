
#ifndef LIBC_H
#define LIBC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* memory */
void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
int   memcmp(const void *a, const void *b, size_t n);

/* string */
size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
char *strcpy(char *dst, const char *src);

/* stdio */
int putchar(int c);
int puts(const char *s);
int printf(const char *fmt, ...);

/* stdlib */
int atoi(const char *s);

/* ctype */
int isdigit(int c);
int isalpha(int c);

#endif
