/*
Based on: https://chromium.googlesource.com/native_client/nacl-newlib/+/10986ed23493d1d252fb9c2c820c20cbfa00de9a/newlib/libc/locale/locale.c
*/

#include <limits.h>
#include <locale.h>

static const struct lconv lconv_data = 
{
  ".", "", "", "", "", "", "", "", "", "",
  CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
  CHAR_MAX, CHAR_MAX, CHAR_MAX, CHAR_MAX,
};

struct lconv *localeconv(void) {
    return (struct lconv *) &lconv_data;
}
