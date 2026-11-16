#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "A51.h"          /* bo sinh khoa + hexval, parse_key */
#include "mahoa.h"  
#include "giaima.h"  
#include <string.h>

#define SIZEX LX
#define SIZEY LY
#define SIZEZ LZ

/* ---- Ham ho tro cho giao dien: doi hex/bin <-> byte, kiem tra K ---- */
/* chuoi hex (cho phep khoang trang) -> byte; tra ve so byte, -1 neu sai */
static long a51_hex_to_bytes(const char *hex, unsigned char *out)
{
    long n = 0;
    int hi = -1;
    for (; *hex; hex++) {
        int h;
        if (*hex == ' ' || *hex == '\t' || *hex == '\r' || *hex == '\n') continue;
        h = hexval((unsigned char)*hex);
        if (h < 0) return -1;
        if (hi < 0) hi = h; else { out[n++] = (unsigned char)((hi << 4) | h); hi = -1; }
    }
    return (hi >= 0) ? -1 : n;
}

/* chuoi nhi phan 0/1 (cho phep khoang trang) -> byte, SO BIT BAT KY.
 * Bit dau la bit cao cua byte dau; byte cuoi neu thieu bit thi dem 0 vao cuoi.
 * *nbits = so bit that. Tra ve so byte (lam tron len), -1 neu sai */
static long a51_bin_to_bytes(const char *s, unsigned char *out, size_t *nbits)
{
    size_t nb = 0;
    memset(out, 0, strlen(s) / 8 + 2);
    for (; *s; s++) {
        if (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') continue;
        if (*s != '0' && *s != '1') return -1;
        if (*s == '1') out[nb / 8] |= (unsigned char)(0x80 >> (nb % 8));
        nb++;
    }
    *nbits = nb;
    return (long)((nb + 7) / 8);
}

/* byte -> hex viet hoa (sep != 0: cach nhau dau cach); out can len*3+1 ky tu */
static void a51_bytes_to_hex(const unsigned char *in, size_t len, char *out, int sep)
{
    static const char H[] = "0123456789ABCDEF";
    size_t i, p = 0;
    for (i = 0; i < len; i++) {
        out[p++] = H[in[i] >> 4];
        out[p++] = H[in[i] & 15];
        if (sep && i + 1 < len) out[p++] = ' ';
    }
    out[p] = 0;
}

/* nbits bit dau -> chuoi 0/1, cu 8 bit cach nhau 1 dau cach; out can nbits + nbits/8 + 1 ky tu */
static void a51_bits_to_bin(const unsigned char *in, size_t nbits, char *out)
{
    size_t i, p = 0;
    for (i = 0; i < nbits; i++) {
        out[p++] = (char)('0' + ((in[i / 8] >> (7 - i % 8)) & 1));
        if (i % 8 == 7 && i + 1 < nbits) out[p++] = ' ';
    }
    out[p] = 0;
}

/* 3 thanh ghi -> chuoi X:/Y:/Z: de hien thi */
static void a51_state_to_str(const int *x, const int *y, const int *z, char *out)
{
    int j, p = 0;
    p += sprintf(out + p, "X: "); for (j = 0; j < SIZEX; j++) out[p++] = (char)('0' + x[j]);
    p += sprintf(out + p, "\r\nY: "); for (j = 0; j < SIZEY; j++) out[p++] = (char)('0' + y[j]);
    p += sprintf(out + p, "\r\nZ: "); for (j = 0; j < SIZEZ; j++) out[p++] = (char)('0' + z[j]);
    out[p] = 0;
}

/* ------------------------- GDI+ (flat API) ------------------------- */
typedef struct {
    UINT32 GdiplusVersion;
    void  *DebugEventCallback;
    BOOL   SuppressBackgroundThread;
    BOOL   SuppressExternalCodecs;
} GdiplusStartupInputC;

int  WINAPI GdiplusStartup(ULONG_PTR *token, const GdiplusStartupInputC *in, void *out);
void WINAPI GdiplusShutdown(ULONG_PTR token);
int  WINAPI GdipCreateBitmapFromStream(IStream *stream, void **bitmap);
int  WINAPI GdipCreateFromHDC(HDC hdc, void **graphics);
int  WINAPI GdipDrawImageRectI(void *graphics, void *image, int x, int y, int w, int h);
int  WINAPI GdipDeleteGraphics(void *graphics);
int  WINAPI GdipDisposeImage(void *image);
int  WINAPI GdipGetImageWidth(void *image, UINT *w);
int  WINAPI GdipGetImageHeight(void *image, UINT *h);

/* ------------------------------ IDs -------------------------------- */
enum {
    IDC_K = 101, IDC_RTEXT, IDC_RHEX, IDC_RBIN, IDC_BFILE, IDC_FINFO, IDC_IN,
    IDC_BENC, IDC_BDEC, IDC_BSAVE, IDC_BCLEAR, IDC_OUT, IDC_OUTHEX,
    IDC_REG, IDC_STATUS, IDC_PREV
};

/* Gioi han so byte hien thi trong cac o ket qua hex / nhi phan */
#define SHOW_MAX 512

/* Gioi han kich thuoc tep chen vao: 200 KB */
#define FILE_MAX_BYTES (200 * 1024)

/* ---------------------------- Trang thai --------------------------- */
static HINSTANCE g_inst;
static HWND g_wnd, g_hK, g_hRText, g_hRHex, g_hRBin, g_hFInfo, g_hIn, g_hOut,
            g_hOutHex, g_hReg, g_hStatus, g_hPrev, g_hSave;
static HFONT g_fontUI, g_fontMono;
static ULONG_PTR g_gdipToken;

static unsigned char *g_fdata;  static size_t g_flen;  static WCHAR g_fpath[MAX_PATH];
static unsigned char *g_odata;  static size_t g_olen, g_obits;  static WCHAR g_defName[MAX_PATH];

static void  *g_img;            /* GDI+ image dang xem truoc */
static IStream *g_imgStream;
static WCHAR g_prevMsg[128] = L"Chưa có ảnh";

/* ------------------------------ Tien ich --------------------------- */
static void SetStatus(const WCHAR *s) { SetWindowTextW(g_hStatus, s); }

static void Err(const WCHAR *s)
{
    MessageBoxW(g_wnd, s, L"A5/1", MB_OK | MB_ICONWARNING);
    SetStatus(s);
}

static WCHAR *GetText(HWND h)
{
    int n = GetWindowTextLengthW(h);
    WCHAR *b = (WCHAR *)malloc((size_t)(n + 1) * sizeof(WCHAR));
    if (!b) return NULL;
    GetWindowTextW(h, b, n + 1);
    return b;
}

/* wide -> ASCII bo khoang trang; tra ve 0 neu co ky tu ngoai ASCII */
static int WideToAsciiNoSpace(const WCHAR *w, char *out, size_t cap)
{
    size_t p = 0;
    for (; *w; w++) {
        if (*w == L' ' || *w == L'\t' || *w == L'\r' || *w == L'\n') continue;
        if (*w > 127 || p + 1 >= cap) return 0;
        out[p++] = (char)*w;
    }
    out[p] = 0;
    return 1;
}

/* wide -> ASCII (giu nguyen khoang trang); NULL neu co ky tu ngoai ASCII */
static char *WideToAsciiAlloc(const WCHAR *w)
{
    size_t i, n = wcslen(w);
    char *a = (char *)malloc(n + 1);
    if (!a) return NULL;
    for (i = 0; i <= n; i++) {
        if (w[i] > 127) { free(a); return NULL; }
        a[i] = (char)w[i];
    }
    return a;
}

static WCHAR *AsciiToWide(const char *s)
{
    size_t n = strlen(s), i;
    WCHAR *w = (WCHAR *)malloc((n + 1) * sizeof(WCHAR));
    if (!w) return NULL;
    for (i = 0; i <= n; i++) w[i] = (WCHAR)(unsigned char)s[i];
    return w;
}

static void SetTextAscii(HWND h, const char *s)
{
    WCHAR *w = AsciiToWide(s);
    if (w) { SetWindowTextW(h, w); free(w); }
}

static void SetUIFont(HWND h, HFONT f) { SendMessageW(h, WM_SETFONT, (WPARAM)f, TRUE); }

static BOOL CALLBACK SetFontProc(HWND h, LPARAM lp) { SetUIFont(h, (HFONT)lp); return TRUE; }

static const WCHAR *BaseName(const WCHAR *p)
{
    const WCHAR *s = wcsrchr(p, L'\\');
    return s ? s + 1 : p;
}

/* ------------------------- Xem truoc anh (GDI+) -------------------- */
static void ClearImage(void)
{
    if (g_img)       { GdipDisposeImage(g_img); g_img = NULL; }
    if (g_imgStream) { g_imgStream->lpVtbl->Release(g_imgStream); g_imgStream = NULL; }
}

static void SetPreview(const unsigned char *data, size_t len, const WCHAR *msgIfFail)
{
    ClearImage();
    wcscpy(g_prevMsg, msgIfFail);
    if (data && len > 0) {
        HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, len);
        if (hg) {
            void *p = GlobalLock(hg);
            memcpy(p, data, len);
            GlobalUnlock(hg);
            IStream *st = NULL;
            if (CreateStreamOnHGlobal(hg, TRUE, &st) == S_OK) {
                void *img = NULL;
                if (GdipCreateBitmapFromStream(st, &img) == 0 && img) {
                    g_img = img; g_imgStream = st;
                } else {
                    st->lpVtbl->Release(st);
                }
            } else {
                GlobalFree(hg);
            }
        }
    }
    InvalidateRect(g_hPrev, NULL, TRUE);
}

static LRESULT CALLBACK PreviewProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        FillRect(dc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
        if (g_img) {
            UINT iw = 0, ih = 0;
            GdipGetImageWidth(g_img, &iw); GdipGetImageHeight(g_img, &ih);
            if (iw && ih) {
                double sx = (double)(rc.right - 8) / iw, sy = (double)(rc.bottom - 8) / ih;
                double s = sx < sy ? sx : sy;
                if (s > 1.0) s = 1.0;
                int dw = (int)(iw * s), dh = (int)(ih * s);
                void *g = NULL;
                if (GdipCreateFromHDC(dc, &g) == 0) {
                    GdipDrawImageRectI(g, g_img, (rc.right - dw) / 2, (rc.bottom - dh) / 2, dw, dh);
                    GdipDeleteGraphics(g);
                }
            }
        } else {
            SelectObject(dc, g_fontUI);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(120, 120, 120));
            DrawTextW(dc, g_prevMsg, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        EndPaint(h, &ps);
        return 0;
    }
    if (m == WM_ERASEBKGND) return 1;
    return DefWindowProcW(h, m, w, l);
}

/* ---------------------------- Tep -------------------------------- */
static unsigned char *ReadAll(const WCHAR *path, size_t *len)
{
    HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return NULL;
    LARGE_INTEGER sz;
    if (!GetFileSizeEx(f, &sz) || sz.QuadPart <= 0 || sz.QuadPart > FILE_MAX_BYTES) {
        CloseHandle(f); return NULL;
    }
    unsigned char *b = (unsigned char *)malloc((size_t)sz.QuadPart);
    if (!b) { CloseHandle(f); return NULL; }
    size_t done = 0;
    while (done < (size_t)sz.QuadPart) {
        DWORD got = 0, want = (DWORD)(((size_t)sz.QuadPart - done) > 0x4000000 ? 0x4000000 : ((size_t)sz.QuadPart - done));
        if (!ReadFile(f, b + done, want, &got, NULL) || got == 0) { free(b); CloseHandle(f); return NULL; }
        done += got;
    }
    CloseHandle(f);
    *len = done;
    return b;
}

static int WriteAll(const WCHAR *path, const unsigned char *d, size_t len)
{
    HANDLE f = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return 0;
    size_t done = 0;
    while (done < len) {
        DWORD put = 0, want = (DWORD)((len - done) > 0x4000000 ? 0x4000000 : (len - done));
        if (!WriteFile(f, d + done, want, &put, NULL) || put == 0) { CloseHandle(f); return 0; }
        done += put;
    }
    CloseHandle(f);
    return 1;
}

/* Kiem tra ban ma hop le UTF-8, in duoc */
static int Utf8Printable(const unsigned char *d, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        if ((d[i] < 32 && d[i] != '\r' && d[i] != '\n' && d[i] != '\t') || d[i] == 127) return 0;
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (const char *)d, (int)n, NULL, 0) > 0;
}

/* UTF-8 -> wide, doi \n don thanh \r\n. Tra ve chuoi malloc. */
static WCHAR *Utf8ToEditText(const unsigned char *d, size_t n)
{
    int wn = MultiByteToWideChar(CP_UTF8, 0, (const char *)d, (int)n, NULL, 0);
    WCHAR *w = (WCHAR *)malloc((size_t)(wn + 1) * sizeof(WCHAR));
    WCHAR *o = (WCHAR *)malloc((size_t)(wn * 2 + 1) * sizeof(WCHAR));
    int i, p = 0;
    if (!w || !o) { free(w); free(o); return NULL; }
    MultiByteToWideChar(CP_UTF8, 0, (const char *)d, (int)n, w, wn);
    for (i = 0; i < wn; i++) {
        if (w[i] == L'\n' && (i == 0 || w[i - 1] != L'\r')) o[p++] = L'\r';
        o[p++] = w[i];
    }
    o[p] = 0;
    free(w);
    return o;
}

/* ---------------------------- Chuc nang ----------------------------- */
static void ClearFile(void)
{
    free(g_fdata); g_fdata = NULL; g_flen = 0; g_fpath[0] = 0;
    SetWindowTextW(g_hFInfo, L"Chưa chèn tệp (đang dùng nội dung gõ vào ô bên dưới)");
    SendMessageW(g_hIn, EM_SETREADONLY, FALSE, 0);
}

static void OnInsertFile(void)
{
    WCHAR path[MAX_PATH] = L"";
    OPENFILENAMEW o; memset(&o, 0, sizeof o);
    o.lStructSize = sizeof o; o.hwndOwner = g_wnd;
    o.lpstrFilter = L"Tất cả tệp (*.*)\0*.*\0Văn bản (*.txt)\0*.txt\0Ảnh (*.png;*.jpg;*.jpeg;*.bmp;*.gif)\0*.png;*.jpg;*.jpeg;*.bmp;*.gif\0Tệp đã mã hóa (*.a51)\0*.a51\0\0";
    o.lpstrFile = path; o.nMaxFile = MAX_PATH;
    o.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&o)) return;

    /* kiem tra kich thuoc truoc khi doc: toi da 200 KB */
    {
        WIN32_FILE_ATTRIBUTE_DATA fa;
        if (GetFileAttributesExW(path, GetFileExInfoStandard, &fa)) {
            ULONGLONG fsz = ((ULONGLONG)fa.nFileSizeHigh << 32) | fa.nFileSizeLow;
            if (fsz == 0) { Err(L"Tệp rỗng."); return; }
            if (fsz > FILE_MAX_BYTES) {
                Err(L"Tệp quá lớn: chỉ chấp nhận tệp có kích thước ≤ 200 KB.");
                return;
            }
        }
    }

    size_t len = 0;
    unsigned char *d = ReadAll(path, &len);
    if (!d) { Err(L"Không đọc được tệp (tệp đang bị khóa hoặc không truy cập được)."); return; }

    ClearFile();
    g_fdata = d; g_flen = len; wcscpy(g_fpath, path);

    WCHAR info[MAX_PATH + 64];
    swprintf(info, MAX_PATH + 64, L"Đã chèn: %ls  (%llu byte)", BaseName(path), (unsigned long long)len);
    SetWindowTextW(g_hFInfo, info);

    /* Hien thi noi dung vao o nhap neu la van ban nho; con lai hien thi mo ta */
    if (len <= 65536 && Utf8Printable(d, len)) {
        WCHAR *t = Utf8ToEditText(d, len);
        if (t) { SetWindowTextW(g_hIn, t); free(t); }
    } else {
        WCHAR msg[MAX_PATH + 128];
        swprintf(msg, MAX_PATH + 128, L"[Dữ liệu nhị phân từ tệp \"%ls\" - %llu byte]", BaseName(path), (unsigned long long)len);
        SetWindowTextW(g_hIn, msg);
    }
    SendMessageW(g_hIn, EM_SETREADONLY, TRUE, 0);

    SetPreview(d, len, L"Tệp không phải ảnh");
    SetStatus(L"Đã chèn tệp. Nhập khóa K rồi bấm Mã hóa hoặc Giải mã.");
}

static void BuildDefaultName(int encrypt)
{
    if (g_fdata) {
        const WCHAR *b = BaseName(g_fpath);
        size_t n = wcslen(b);
        if (encrypt) swprintf(g_defName, MAX_PATH, L"%ls.a51", b);
        else if (n > 4 && _wcsicmp(b + n - 4, L".a51") == 0) { wcsncpy(g_defName, b, n - 4); g_defName[n - 4] = 0; }
        else swprintf(g_defName, MAX_PATH, L"%ls.giaima", b);
    } else {
        wcscpy(g_defName, encrypt ? L"banma.a51" : L"banro.txt");
    }
}

static void OnCrypt(int encrypt)
{
    char kbuf[128];
    char key[65];
    WCHAR *w;

    /* --- doc K --- */
    w = GetText(g_hK);
    int okK = w && WideToAsciiNoSpace(w, kbuf, sizeof kbuf) && parse_key(kbuf, key);
    free(w);
    if (!okK) { Err(L"Khóa K không hợp lệ: cần đúng 16 ký tự hex (0-9, A-F) hoặc 64 ký tự nhị phân (0/1). Ví dụ hex: 123456789ABCDEF0"); return; }

    /* --- doc du lieu vao --- */
    unsigned char *in = NULL; size_t inlen = 0, nbits = 0; int freeIn = 0;
    if (g_fdata) {
        in = g_fdata; inlen = g_flen;
    } else {
        int mode = SendMessageW(g_hRHex, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 :
                   SendMessageW(g_hRBin, BM_GETCHECK, 0, 0) == BST_CHECKED ? 2 : 0;  /* 0 = van ban */
        w = GetText(g_hIn);
        if (!w || !w[0]) { free(w); Err(L"Chưa có dữ liệu: hãy gõ vào ô nhập hoặc chèn một tệp."); return; }
        if (mode == 1 || mode == 2) {
            char *a = WideToAsciiAlloc(w);
            long got = -1;
            if (a) {
                in = (unsigned char *)malloc(strlen(a) / 2 + 2);
                if (in) got = (mode == 1) ? a51_hex_to_bytes(a, in) : a51_bin_to_bytes(a, in, &nbits);
            }
            free(a);
            if (got <= 0) {
                free(in); free(w);
                Err(mode == 1 ? L"Chuỗi hex không hợp lệ (ký tự lạ hoặc số ký tự lẻ)."
                              : L"Chuỗi nhị phân không hợp lệ (chỉ được chứa ký tự 0 và 1).");
                return;
            }
            inlen = (size_t)got;
        } else {
            if (!encrypt) {
                free(w);
                Err(L"Để giải mã dữ liệu gõ vào, hãy chọn dạng nhập Hex hoặc Nhị phân và dán bản mã (hoặc chèn tệp .a51).");
                return;
            }
            int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL) - 1;
            in = (unsigned char *)malloc((size_t)n + 1);
            WideCharToMultiByte(CP_UTF8, 0, w, -1, (char *)in, n + 1, NULL, NULL);
            inlen = (size_t)n;
        }
        free(w);
        freeIn = 1;
    }

    /* --- chay A5/1: ma hoa hoac giai ma tuy nut bam --- */
    if (nbits == 0) nbits = inlen * 8;          /* van ban / hex / tep: tinh theo byte */
    free(g_odata);
    g_odata = (unsigned char *)malloc(inlen ? inlen : 1);
    g_olen = inlen;
    g_obits = nbits;
    int rx[SIZEX], ry[SIZEY], rz[SIZEZ];
    if (encrypt) ma_hoa (key, in, g_odata, nbits, rx, ry, rz);
    else         giai_ma(key, in, g_odata, nbits, rx, ry, rz);
    if (freeIn) free(in);
    BuildDefaultName(encrypt);

    /* --- hien thi ket qua dang hex --- */
    size_t showBits = g_obits > SHOW_MAX * 8 ? SHOW_MAX * 8 : g_obits;
    size_t showN = (showBits + 7) / 8;
    char *hex = (char *)malloc(showN * 3 + 8);
    a51_bytes_to_hex(g_odata, showN, hex, 1);
    if (g_obits > showBits) strcat(hex, " ...");
    SetTextAscii(g_hOutHex, hex);
    free(hex);

    WCHAR head[MAX_PATH + 256];
    if (g_fdata) {
        swprintf(head, MAX_PATH + 256, L"%ls tệp \"%ls\": %llu byte.\r\nBấm \"Lưu kết quả...\" để ghi ra tệp.\r\n(Hex bên dưới: %llu byte đầu)",
                 encrypt ? L"Đã mã hóa" : L"Đã giải mã", BaseName(g_fpath),
                 (unsigned long long)g_olen, (unsigned long long)showN);
        SetWindowTextW(g_hOut, head);
    } else if (nbits % 8) {
        /* so bit khong chia het cho 8: Hex khong the hien dung, nen ghi thang day bit o day */
        size_t nb = nbits > SHOW_MAX * 8 ? SHOW_MAX * 8 : nbits;
        char *bt = (char *)malloc(nb + nb / 8 + 8);
        a51_bits_to_bin(g_odata, nb, bt);
        WCHAR *btw = AsciiToWide(bt);
        size_t cap = btw ? wcslen(btw) + 128 : 128;
        WCHAR *hb = (WCHAR *)malloc(cap * sizeof(WCHAR));
        if (btw && hb) {
            swprintf(hb, cap, L"%ls (%llu bit): %ls%ls\r\n",
                     encrypt ? L"Bản mã" : L"Bản rõ", (unsigned long long)nbits, btw, nbits > nb ? L" ..." : L"");
            SetWindowTextW(g_hOut, hb);
        }
        free(bt); free(btw); free(hb);
    } else if (encrypt) {
        swprintf(head, MAX_PATH + 256, L"Bản mã: %llu byte (xem dạng Hex bên dưới).",
                 (unsigned long long)g_olen);
        SetWindowTextW(g_hOut, head);
    } else if (Utf8Printable(g_odata, g_olen)) {
        WCHAR *t = Utf8ToEditText(g_odata, g_olen);
        if (t) { SetWindowTextW(g_hOut, t); free(t); }
    } else {
        swprintf(head, MAX_PATH + 256, L"Kết quả không phải văn bản UTF-8 (có thể sai khóa K): %llu byte.\r\nXem dạng Hex bên dưới.",
                 (unsigned long long)g_olen);
        SetWindowTextW(g_hOut, head);
    }

    /* --- khung: thanh ghi X, Y, Z sau khi quay xong --- */
    {
        char reg[256];
        a51_state_to_str(rx, ry, rz, reg);
        WCHAR *regW = AsciiToWide(reg);
        size_t cap = regW ? wcslen(regW) + 96 : 0;
        WCHAR *txt = cap ? (WCHAR *)malloc(cap * sizeof(WCHAR)) : NULL;
        if (txt) {
            swprintf(txt, cap, L"Thanh ghi sau khi quay xong %llu bit:\r\n%ls",
                     (unsigned long long)nbits, regW);
            SetWindowTextW(g_hReg, txt);
        }
        free(regW); free(txt);
    }

    /* --- xem truoc anh cua ket qua --- */
    if (g_fdata) SetPreview(g_odata, g_olen, encrypt ? L"Bản mã - không phải ảnh hợp lệ" : L"Kết quả không phải ảnh (sai khóa?)");

    EnableWindow(g_hSave, TRUE);
    SetStatus(encrypt ? L"Mã hóa xong." : L"Giải mã xong.");
}

static void OnSave(void)
{
    if (!g_odata) { Err(L"Chưa có kết quả để lưu."); return; }
    WCHAR path[MAX_PATH];
    wcscpy(path, g_defName);
    OPENFILENAMEW o; memset(&o, 0, sizeof o);
    o.lStructSize = sizeof o; o.hwndOwner = g_wnd;
    o.lpstrFilter = L"Tất cả tệp (*.*)\0*.*\0\0";
    o.lpstrFile = path; o.nMaxFile = MAX_PATH;
    o.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (!GetSaveFileNameW(&o)) return;
    if (WriteAll(path, g_odata, g_olen)) SetStatus(L"Đã lưu kết quả ra tệp.");
    else Err(L"Không ghi được tệp.");
}

static void OnClear(void)
{
    ClearFile();
    free(g_odata); g_odata = NULL; g_olen = 0;
    SetWindowTextW(g_hIn, L"");
    SetWindowTextW(g_hOut, L"");
    SetWindowTextW(g_hOutHex, L"");
    SetWindowTextW(g_hReg, L"");
    EnableWindow(g_hSave, FALSE);
    SetPreview(NULL, 0, L"Chưa có ảnh");
    SetStatus(L"Đã xóa dữ liệu (giữ nguyên K).");
}

/* ------------------------------ Giao dien --------------------------- */
static HWND Mk(const WCHAR *cls, const WCHAR *txt, DWORD style, DWORD ex,
               int x, int y, int w, int h, int id)
{
    return CreateWindowExW(ex, cls, txt, WS_CHILD | WS_VISIBLE | style, x, y, w, h,
                           g_wnd, (HMENU)(INT_PTR)id, g_inst, NULL);
}

static void BuildUI(void)
{
    const DWORD EDITSTYLE = ES_AUTOHSCROLL;
    const DWORD MULTI = ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_WANTRETURN;

    /* ---- cot trai ---- */
    Mk(L"STATIC", L"Khóa K (16 ký tự hex hoặc 64 bit 0/1):", 0, 0, 15, 12, 530, 18, 0);
    g_hK = Mk(L"EDIT", L"", EDITSTYLE | WS_TABSTOP, WS_EX_CLIENTEDGE, 15, 32, 530, 26, IDC_K);
    SendMessageW(g_hK, EM_SETLIMITTEXT, 100, 0);

    Mk(L"STATIC", L"Dạng nhập:", 0, 0, 15, 74, 85, 18, 0);
    g_hRText = Mk(L"BUTTON", L"Văn bản", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 0, 100, 72, 80, 22, IDC_RTEXT);
    g_hRHex  = Mk(L"BUTTON", L"Hex", BS_AUTORADIOBUTTON, 0, 185, 72, 55, 22, IDC_RHEX);
    g_hRBin  = Mk(L"BUTTON", L"Nhị phân", BS_AUTORADIOBUTTON, 0, 245, 72, 90, 22, IDC_RBIN);
    SendMessageW(g_hRText, BM_SETCHECK, BST_CHECKED, 0);
    Mk(L"BUTTON", L"Chèn tệp (văn bản / ảnh)...", BS_PUSHBUTTON | WS_TABSTOP, 0, 345, 66, 200, 28, IDC_BFILE);

    g_hFInfo = Mk(L"STATIC", L"", SS_LEFTNOWORDWRAP, 0, 15, 102, 530, 18, IDC_FINFO);
    SetWindowTextW(g_hFInfo, L"Chưa chèn tệp (đang dùng nội dung gõ vào ô bên dưới)");

    Mk(L"STATIC", L"Bản rõ / bản mã đầu vào:", 0, 0, 15, 126, 330, 18, 0);
    g_hIn = Mk(L"EDIT", L"", MULTI | WS_TABSTOP, WS_EX_CLIENTEDGE, 15, 146, 530, 96, IDC_IN);
    SendMessageW(g_hIn, EM_SETLIMITTEXT, 0x7FFFFFFE, 0);

    Mk(L"BUTTON", L"Mã hóa", BS_PUSHBUTTON | WS_TABSTOP, 0, 15, 252, 120, 32, IDC_BENC);
    Mk(L"BUTTON", L"Giải mã", BS_PUSHBUTTON | WS_TABSTOP, 0, 145, 252, 120, 32, IDC_BDEC);
    g_hSave = Mk(L"BUTTON", L"Lưu kết quả...", BS_PUSHBUTTON | WS_TABSTOP, 0, 275, 252, 140, 32, IDC_BSAVE);
    Mk(L"BUTTON", L"Xóa", BS_PUSHBUTTON | WS_TABSTOP, 0, 425, 252, 120, 32, IDC_BCLEAR);
    EnableWindow(g_hSave, FALSE);

    Mk(L"STATIC", L"Kết quả (văn bản / mô tả):", 0, 0, 15, 294, 530, 18, 0);
    g_hOut = Mk(L"EDIT", L"", MULTI | ES_READONLY, WS_EX_CLIENTEDGE, 15, 314, 530, 100, IDC_OUT);

    Mk(L"STATIC", L"Kết quả dạng Hex:", 0, 0, 15, 424, 530, 18, 0);
    g_hOutHex = Mk(L"EDIT", L"", MULTI | ES_READONLY, WS_EX_CLIENTEDGE, 15, 444, 530, 154, IDC_OUTHEX);

    /* ---- cot phai ---- */
    Mk(L"STATIC", L"Xem trước ảnh:", 0, 0, 565, 12, 320, 18, 0);
    g_hPrev = CreateWindowExW(WS_EX_CLIENTEDGE, L"A51Preview", L"", WS_CHILD | WS_VISIBLE,
                              565, 32, 320, 280, g_wnd, (HMENU)(INT_PTR)IDC_PREV, g_inst, NULL);

    Mk(L"STATIC", L"Sau khi quay: thanh ghi X, Y, Z:", 0, 0, 565, 322, 320, 18, 0);
    g_hReg = Mk(L"EDIT", L"", MULTI | ES_READONLY, WS_EX_CLIENTEDGE, 565, 342, 320, 256, IDC_REG);

    g_hStatus = Mk(L"STATIC", L"Nhập khóa K, gõ bản rõ hoặc chèn tệp, rồi bấm Mã hóa / Giải mã.",
                   SS_LEFTNOWORDWRAP, 0, 15, 610, 870, 18, IDC_STATUS);

    EnumChildWindows(g_wnd, SetFontProc, (LPARAM)g_fontUI);
    SetUIFont(g_hK, g_fontMono);
    SetUIFont(g_hIn, g_fontMono);     SetUIFont(g_hOut, g_fontMono);
    SetUIFont(g_hOutHex, g_fontMono); SetUIFont(g_hReg, g_fontMono);
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_CREATE:
        g_wnd = h;
        g_fontUI   = CreateFontW(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        g_fontMono = CreateFontW(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
        BuildUI();
        return 0;
    case WM_COMMAND:
        if (HIWORD(w) == BN_CLICKED || HIWORD(w) == 0) {
            switch (LOWORD(w)) {
            case IDC_BFILE:  OnInsertFile(); break;
            case IDC_BENC:   OnCrypt(1); break;
            case IDC_BDEC:   OnCrypt(0); break;
            case IDC_BSAVE:  OnSave(); break;
            case IDC_BCLEAR: OnClear(); break;
            }
        }
        return 0;
    case WM_CTLCOLORSTATIC:
        if ((HWND)l == g_hOut || (HWND)l == g_hOutHex ||
            (HWND)l == g_hReg || (HWND)l == g_hIn) {
            SetBkColor((HDC)w, GetSysColor(COLOR_WINDOW));
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    case WM_DESTROY:
        ClearImage();
        free(g_fdata); free(g_odata);
        DeleteObject(g_fontUI); DeleteObject(g_fontMono);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    (void)prev; (void)cmd;
    g_inst = inst;

    GdiplusStartupInputC gi = { 1, NULL, FALSE, FALSE };
    GdiplusStartup(&g_gdipToken, &gi, NULL);
    CoInitialize(NULL);

    WNDCLASSEXW pc; memset(&pc, 0, sizeof pc);
    pc.cbSize = sizeof pc; pc.lpfnWndProc = PreviewProc; pc.hInstance = inst;
    pc.hCursor = LoadCursor(NULL, IDC_ARROW); pc.lpszClassName = L"A51Preview";
    RegisterClassExW(&pc);

    WNDCLASSEXW wc; memset(&wc, 0, sizeof wc);
    wc.cbSize = sizeof wc; wc.lpfnWndProc = WndProc; wc.hInstance = inst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"A51Main";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    RECT rc = { 0, 0, 900, 640 };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rc, style, FALSE);
    HWND hw = CreateWindowExW(0, L"A51Main", L"Mã hóa / Giải mã A5/1", style,
                              CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
                              NULL, NULL, inst, NULL);
    ShowWindow(hw, show);
    UpdateWindow(hw);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(hw, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    GdiplusShutdown(g_gdipToken);
    CoUninitialize();
    return (int)msg.wParam;
}
