#ifndef A51_H
#define A51_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define LX 19
#define LY 22
#define LZ 23

/* Ham chiem da so */
int maj(int a, int b, int c){
    return (a + b + c >= 2) ? 1 : 0;
}

/* Quay X: t = x13 ^ x16 ^ x17 ^ x18 */
void Quay_X(int X[]){
    int t = X[13] ^ X[16] ^ X[17] ^ X[18], j;
    for (j = 18; j >= 1; j--) X[j] = X[j - 1];
    X[0] = t;
}
/* Quay Y: t = y20 ^ y21 */
void Quay_Y(int Y[]){
    int t = Y[20] ^ Y[21], j;
    for (j = 21; j >= 1; j--) Y[j] = Y[j - 1];
    Y[0] = t;
}
/* Quay Z: t = z7 ^ z20 ^ z21 ^ z22 */
void Quay_Z(int Z[]){
    int t = Z[7] ^ Z[20] ^ Z[21] ^ Z[22], j;
    for (j = 22; j >= 1; j--) Z[j] = Z[j - 1];
    Z[0] = t;
}

/* Chia 64 bit khoa (19 + 22 + 23) vao X, Y, Z */
void chia_khoa(const char *khoa, int X[], int Y[], int Z[]){
    int i, p = 0;
    for (i = 0; i < LX; i++) X[i] = khoa[p++] - '0';
    for (i = 0; i < LY; i++) Y[i] = khoa[p++] - '0';
    for (i = 0; i < LZ; i++) Z[i] = khoa[p++] - '0';
    /* - '0' la chuyen doi ky tu sang so nguyen */
}

/* Sinh 1 bit khoa dong */
int sinh_bit(int X[], int Y[], int Z[]){
    int m = maj(X[8], Y[10], Z[10]), s;

    if (X[8] == m) Quay_X(X);
    if (Y[10] == m) Quay_Y(Y);
    if (Z[10] == m) Quay_Z(Z);

    s = X[18] ^ Y[21] ^ Z[22];
    return s;
}

/* Ky tu hex -> so 0..15, sai tra ve -1 */
int hexval(char c){
    if (c >= '0' && c <= '9') return c - '0';
    c |= 32;                              /* doi ve chu thuong */
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

/* K: 64 ky tu 0/1 HOAC 16 ky tu hex -> chuoi 64 ky tu 0/1. Dung tra ve 1, sai tra ve 0 */
int parse_key(const char *s, char bits[65]){
    int i, b, n = strlen(s);
    if (n == 64){
        for (i = 0; i < 64; i++){
            if (s[i] != '0' && s[i] != '1') return 0;
            bits[i] = s[i];
        }
    } else if (n == 16){
        for (i = 0; i < 16; i++){
            int v = hexval(s[i]);
            if (v < 0) return 0;
            for (b = 0; b < 4; b++) bits[4*i + b] = '0' + ((v >> (3 - b)) & 1);
        }
    } else return 0;
    bits[64] = 0;
    return 1;
}

#endif /* A51_H */
