/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright 2021-2022 Martin Åberg */

/*
 * Limitations:
 * - void * is converted to unsigned long before format.
 */

#include <limits.h>
#include <priv.h>

#define FLAG_LEFT       0x01
#define FLAG_SIGN       0x02
#define FLAG_SPACE      0x04
#define FLAG_ZERO       0x08
#define FLAG_LONG       0x10
#define FLAG_MINUS      0x20

union charlens {
  char ulong [sizeof (unsigned long) * CHAR_BIT / 3];
  char cvoidp[sizeof (void *)        * CHAR_BIT / 4];
};

/* Maybe fill with ' ' or '0' to the left. */
static int right_adjust(
  void (*put)(void *arg, int c),
  void *arg,
  int flags,
  int width
) {
  if ((flags & FLAG_LEFT) == 0) {
    int t;

    t = flags & FLAG_ZERO ? '0' : ' ';
    while (0 < width) {
      put(arg, t);
      width--;
    }
  }

  return width;
}

/* Maybe add space to the right. */
static void left_adjust(
  void (*put)(void *arg, int c),
  void *arg,
  int flags,
  int width
) {
  if (flags & FLAG_LEFT) {
    while (0 < width) {
      put(arg, ' ');
      width--;
    }
  }
}

static void putul(
  void (*put)(void *arg, int c),
  void *arg,
  unsigned long val,
  int base,
  int flags,
  int width
) {
  char buf[sizeof (union charlens) + 1];
  char *p;

  /* Encode to buf, least significant digit first. */
  p = &buf[0];
  while (1) {
    *p++ = "0123456789abcdef"[val % base];
    val /= base;
    if (val == 0) {
      break;
    }
  }

  if (flags & FLAG_MINUS) {
    *p++ = '-';
  } else if (flags & FLAG_SIGN) {
    *p++ = '+';
  } else if (flags & FLAG_SPACE) {
    *p++ = ' ';
  }

  /* Number of additional chars to fill. Negative means none. */
  width -= p - buf;

  width = right_adjust(put, arg, flags, width);
  while (1) {
    p--;
    put(arg, *p);
    if (p <= buf) {
      break;
    }
  }
  left_adjust(put, arg, flags, width);
}

void iRawDoFmt(
  Lib *lib,
  void (*put)(void *arg, int c),
  void *arg,
  const char *fmt,
  va_list ap
) {
  while (1) {
    int base;
    int flags;
    int width;
    int c;

    c = *fmt++;
    if (c == '\0') {
      return;
    }
    if (c != '%') {
      put(arg, c);
      continue;
    }

    /* Parse flags. */
    flags = 0;
    while (1) {
      c = *fmt++;
      switch (c) {
        case '-': flags |= FLAG_LEFT;  continue;
        case '+': flags |= FLAG_SIGN;  continue;
        case ' ': flags |= FLAG_SPACE; continue;
        case '0': flags |= FLAG_ZERO;  continue;
      }
      break;
    }

    /* Parse minimum field width. */
    width = 0;
    while (1) {
      if (c < '0' || '9' < c) {
        break;
      }
      width *= 10;
      width += c - '0';
      c = *fmt++;
    }

    /* Parse length modifier. */
    if (c == 'l') {
      flags |= FLAG_LONG;
      c = *fmt++;
    }

    /* Parse conversion character. */
    base = 8;
    switch (c) {
      case 'd': {
          unsigned long val;
          long sval;

          if (flags & FLAG_LONG) {
            sval = va_arg(ap, long);
          } else {
            sval = va_arg(ap, int);
          }
          val = sval;
          if (sval < 0) {
            val = -sval;
            flags |= FLAG_MINUS;
          }
          putul(put, arg, val, 10, flags, width);
        }
        break;

      case 'x':
      case 'X':
        base += 6;
      case 'u':
        base += 2;
      case 'o': {
          unsigned long val;

          if (flags & FLAG_LONG) {
            val = va_arg(ap, unsigned long);
          } else {
            val = va_arg(ap, unsigned int);
          }
          putul(put, arg, val, base, flags, width);
        }
        break;

      case 'c':
        c = (unsigned char) va_arg(ap, int);
        put(arg, c);
        break;

      case 's': {
          char *val;
          char *q;

          val = va_arg(ap, char *);
          q = val;
          while (*q != '\0') {
            q++;
          }
          width -= q - val;
          width = right_adjust(put, arg, flags, width);
          while (1) {
            c = *val++;
            if (c == '\0') {
              break;
            }
            put(arg, c);
          }
          left_adjust(put, arg, flags, width);
        }
        break;

      case 'p': {
          unsigned long val;

          val = (unsigned long) va_arg(ap, void *);
          putul(
            put,
            arg,
            val,
            16,
            flags | FLAG_ZERO,
            sizeof (void *) * CHAR_BIT / 4
          );
        }
        break;

      default:
        put(arg, c);
        break;

      case '\0':
        return;
    }
  }
}

