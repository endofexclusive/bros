#include <stddef.h>

void *memcpy(void *restrict dst0, const void *restrict src0, size_t n);
void *memcpy(void *restrict dst0, const void *restrict src0, size_t n) {
  char *dst = (char *) dst0;
  char *src = (char *) src0;
  while (n--) {
    *dst++ = *src++;
  }
  return dst0;
}

