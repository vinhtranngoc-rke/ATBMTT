#ifndef A51_ENCRYPT_H
#define A51_ENCRYPT_H

#include "A51.h"

void ma_hoa(const char *khoa, const unsigned char *P, unsigned char *C, size_t nbits,
            int X[], int Y[], int Z[]){
    size_t i;

    chia_khoa(khoa, X, Y, Z);            /* B1: nap khoa vao 3 thanh ghi */
    memset(C, 0, (nbits + 7) / 8);       /* B2: xoa C ve toan bit 0 */

    for (i = 0; i < nbits; i++){
        int byte = (int)(i / 8);         /* bit thu i nam o byte so may */
        int pos  = 7 - (int)(i % 8);     /* vi tri trong byte (bit cao dem truoc) */

        int p = (P[byte] >> pos) & 1;    /* B3: lay bit ban ro thu i */
        int s = sinh_bit(X, Y, Z);       /* B4: sinh 1 bit khoa */
        int c = p ^ s;                   /* B5: C = P XOR S */

        if (c == 1) C[byte] |= (unsigned char)(1 << pos);   /* B6: ghi bit ban ma */
    }
}

#endif /* A51_ENCRYPT_H */
