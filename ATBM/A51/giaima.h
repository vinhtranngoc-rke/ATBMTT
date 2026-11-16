#ifndef A51_DECRYPT_H
#define A51_DECRYPT_H

#include "A51.h"

/* GIAI MA: P = C XOR S  (cung day bit khoa S nhu luc ma hoa, nen phai dung dung khoa K)
 *   khoa : chuoi 64 ky tu '0'/'1' (da qua parse_key)
 *   C    : ban ma vao (nbits bit)
 *   P    : ban ro ra, phai co it nhat (nbits + 7) / 8 byte
 *   X,Y,Z: 3 thanh ghi; ham tra ve trang thai cuoi sau khi quay xong */
void giai_ma(const char *khoa, const unsigned char *C, unsigned char *P, size_t nbits,
             int X[], int Y[], int Z[]){
    size_t i;

    chia_khoa(khoa, X, Y, Z);            /* B1: nap lai khoa vao 3 thanh ghi */
    memset(P, 0, (nbits + 7) / 8);       /* B2: xoa P ve toan bit 0 */

    for (i = 0; i < nbits; i++){
        int byte = (int)(i / 8);
        int pos  = 7 - (int)(i % 8);

        int c = (C[byte] >> pos) & 1;    /* B3: lay bit ban ma thu i */
        int s = sinh_bit(X, Y, Z);       /* B4: sinh 1 bit khoa */
        int p = c ^ s;                   /* B5: P = C XOR S */

        if (p == 1) P[byte] |= (unsigned char)(1 << pos);   /* B6: ghi bit ban ro */
    }
}

#endif /* A51_DECRYPT_H */
