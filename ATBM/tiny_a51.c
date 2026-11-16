#include <stdio.h>
#include <string.h>

#define MAX_BIT 100
#define LX 6
#define LY 8
#define LZ 9

/* Ham chiem da so */
int maj(int a, int b, int c){
    return (a + b + c >= 2) ? 1 : 0;
}

/* Quay X: t = x2 ^ x4 ^ x5 */
void Quay_X(int X[]){
    int t = X[2] ^ X[4] ^ X[5], j;
    printf("Quay X: t = x2^x4^x5 = %d^%d^%d = %d\n", X[2], X[4], X[5], t);
    for (j = 5; j >= 1; j--) X[j] = X[j - 1];
    X[0] = t;
}
/* Quay Y: t = y6 ^ y7 */
void Quay_Y(int Y[]){
    int t = Y[6] ^ Y[7], j;
    printf("Quay Y: t = y6^y7 = %d^%d = %d\n", Y[6], Y[7], t);
    for (j = 7; j >= 1; j--) Y[j] = Y[j - 1];
    Y[0] = t;
}
/* Quay Z: t = z2 ^ z7 ^ z8 */
void Quay_Z(int Z[]){
    int t = Z[2] ^ Z[7] ^ Z[8], j;
    printf("Quay Z: t = z2^z7^z8 = %d^%d^%d = %d\n", Z[2], Z[7], Z[8], t);
    for (j = 8; j >= 1; j--) Z[j] = Z[j - 1];
    Z[0] = t;
}

/* Chia 23 bit khoa vao X, Y, Z */
void chia_khoa(const char *khoa, int X[], int Y[], int Z[]){
    int i, p = 0;
    for (i = 0; i < LX; i++) X[i] = khoa[p++] - '0';
    for (i = 0; i < LY; i++) Y[i] = khoa[p++] - '0';
    for (i = 0; i < LZ; i++) Z[i] = khoa[p++] - '0';
    /* - '0' la chuyen doi ky tu sang so nguyen */
}

/* In 3 thanh ghi tren 1 dong */
void thanh_ghi(int X[], int Y[], int Z[]){
    int i;
    printf("X = ");
    for (i = 0; i < LX; i++) printf("%d", X[i]);
    printf("\n");
    printf("Y = ");
    for (i = 0; i < LY; i++) printf("%d", Y[i]);
    printf("\n");
    printf("Z = ");
    for (i = 0; i < LZ; i++) printf("%d", Z[i]);
    printf("\n");
}

/* Sinh 1 bit khoa dong, in cac buoc trung gian */
int sinh_bit(int X[], int Y[], int Z[]){
    int m = maj(X[1], Y[3], Z[3]), s;

    printf("Truoc:\n"); thanh_ghi(X, Y, Z);
    printf("m = maj(x1=%d, y3=%d, z3=%d) = %d\n", X[1], Y[3], Z[3], m);
    printf("\n");

    if (X[1] == m) Quay_X(X); 
    else printf("X giu nguyen (x1=%d != m)\n", X[1]);
    if (Y[3] == m) Quay_Y(Y); 
    else printf("Y giu nguyen (y3=%d != m)\n", Y[3]);
    if (Z[3] == m) Quay_Z(Z); 
    else printf("Z giu nguyen (z3=%d != m)\n", Z[3]);

    printf("\nSau:\n"); 
    thanh_ghi(X, Y, Z);
    s = X[5] ^ Y[7] ^ Z[8];
    printf("s = x5^y7^z8 = %d^%d^%d = %d\n", X[5], Y[7], Z[8], s);
    return s;
}

int main(){
    char khoa[100], text[MAX_BIT + 1];
    int X[LX], Y[LY], Z[LZ], s[MAX_BIT], out[MAX_BIT];
    int mode, n, i, ok;

    printf("===== TinyA5/1 =====\n1. Ma hoa\n2. Giai ma\nChon: ");
    scanf("%d", &mode);

    do{                                   /* nhap khoa 23 bit */
        printf("Nhap khoa 23 bit (0/1): ");
        scanf("%99s", khoa);
        ok = (strlen(khoa) == LX + LY + LZ);
        for (i = 0; ok && i < LX + LY + LZ; i++){
            if (khoa[i] != '0' && khoa[i] != '1') ok = 0;
        }   
        if (ok == 0) printf("Khoa sai! Can dung 23 ky tu 0/1.\n");
    }while (ok == 0);

    do{                                   /* nhap ban ro / ban ma */
        printf("Nhap %s (day bit 0/1, toi da %d bit): ",
               mode == 1 ? "ban ro" : "ban ma", MAX_BIT);
        scanf("%100s", text);
        n = strlen(text);
        ok = (n > 0 && n <= MAX_BIT);
        for (i = 0; ok && i < n; i++){
            if (text[i] != '0' && text[i] != '1') ok = 0;
        }
        if (ok == 0) printf("Sai! Chi nhap cac ky tu 0/1.\n");
    } while (ok == 0);

    chia_khoa(khoa, X, Y, Z);
    printf("\nChia khoa:\n"); thanh_ghi(X, Y, Z);

    for (i = 0; i < n; i++){
        printf("\n--- Buoc %d ---\n", i);
        s[i] = sinh_bit(X, Y, Z);
        out[i] = (text[i] - '0') ^ s[i];
    }

    printf("\n===== KET QUA =====\n");
    printf("%-9s: %s\n", mode == 1 ? "Ban ro P" : "Ban ma C", text);
    printf("Day khoa S: ");
    for (i = 0; i < n; i++) printf("%d", s[i]);
    printf("\n%-9s: ", mode == 1 ? "Ban ma C" : "Ban ro P");
    for (i = 0; i < n; i++) printf("%d", out[i]);
    printf("\n");
    return 0;
}
