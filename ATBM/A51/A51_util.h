#ifndef A51_UTIL_H
#define A51_UTIL_H

#include <stdio.h>
#include <ctype.h>
#include "A51.h"      /* hexval, LX, LY, LZ */

/* Hex (cho phep khoang trang) -> byte. Tra ve so byte, -1 neu sai */
static long hex_to_bytes(const char *s, unsigned char *out)
{
    long k = 0;                            /* dem so ky tu hex */
    int v;
    for (; *s; s++) {
        if (isspace((unsigned char)*s)) continue;
        if ((v = hexval(*s)) < 0) return -1;
        if (k % 2 == 0) out[k / 2] = v << 4;
        else            out[k / 2] |= v;
        k++;
    }
    return (k % 2) ? -1 : k / 2;
}

/* Nhi phan 0/1 (cho phep khoang trang) -> byte, so bit bat ky.
 * *nbits = so bit that. Tra ve so byte (lam tron len), -1 neu sai */
static long bin_to_bytes(const char *s, unsigned char *out, size_t *nbits)
{
    size_t k = 0;                          /* dem so bit */
    for (; *s; s++) {
        if (isspace((unsigned char)*s)) continue;
        if (*s != '0' && *s != '1') return -1;
        if (k % 8 == 0) out[k / 8] = 0;
        if (*s == '1') out[k / 8] |= 0x80 >> (k % 8);
        k++;
    }
    *nbits = k;
    return (long)((k + 7) / 8);
}

/* Byte -> hex viet hoa, cach nhau dau cach; out can len*3+1 ky tu */
static void bytes_to_hex(const unsigned char *in, size_t len, char *out)
{
    size_t i;
    for (i = 0; i < len; i++) sprintf(out + 3 * i, "%02X ", in[i]);
    out[len ? 3 * len - 1 : 0] = 0;        /* bo dau cach cuoi */
}

/* nbits bit dau -> chuoi 0/1, cu 8 bit cach nhau 1 dau cach */
static void bits_to_bin(const unsigned char *in, size_t nbits, char *out)
{
    size_t i, p = 0;
    for (i = 0; i < nbits; i++) {
        if (i > 0 && i % 8 == 0) out[p++] = ' ';
        out[p++] = '0' + ((in[i / 8] >> (7 - i % 8)) & 1);
    }
    out[p] = 0;
}

/* 3 thanh ghi -> chuoi X:/Y:/Z: de hien thi */
static void state_to_str(const int *x, const int *y, const int *z, char *out)
{
    int j, p = sprintf(out, "X: ");
    for (j = 0; j < LX; j++) out[p++] = '0' + x[j];
    p += sprintf(out + p, "\r\nY: ");
    for (j = 0; j < LY; j++) out[p++] = '0' + y[j];
    p += sprintf(out + p, "\r\nZ: ");
    for (j = 0; j < LZ; j++) out[p++] = '0' + z[j];
    out[p] = 0;
}

#endif /* A51_UTIL_H */
