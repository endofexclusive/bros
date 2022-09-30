#include <stddef.h>

void *memset(void *s, int c, size_t n);
void *memset(void *s, int c, size_t n) {
  unsigned char *p;
  p = s;
  while (n--) {
    *p = c;
    p++;
  }
  return s;
}

