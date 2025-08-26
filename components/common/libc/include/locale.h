/*
Based on: https://chromium.googlesource.com/native_client/nacl-newlib/+/10986ed23493d1d252fb9c2c820c20cbfa00de9a/newlib/libc/include/locale.h
*/

#ifndef LOCALE_H
#define LOCALE_H

/* Locale conventions structure */
struct lconv {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
};

struct lconv *localeconv(void);

#endif /* LOCALE_H */
