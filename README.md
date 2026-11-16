# ATBMTT

Các chương trình minh họa thuật toán mã hóa dòng A5/1, gồm phiên bản A5/1 đầy đủ có giao diện và phiên bản TinyA51 phục vụ theo dõi từng bước sinh khóa dòng.

## Cấu trúc thư mục

```text
ATBMTT/
├── README.md
└── ATBM/
    ├── A51/
    │   ├── A51_gui.c   # Chương trình A5/1 có giao diện Windows
    │   ├── A51.h       # Các thanh ghi, thuật toán sinh bit và xử lý khóa
    │   ├── mahoa.h     # Hàm mã hóa
    │   └── giaima.h    # Hàm giải mã
    └── TinyA51/
        └── tiny_a51.c  # Bản TinyA5/1 chạy trên console, in chi tiết từng bước
```

## A51 — giao diện Windows

### Yêu cầu

- Windows.
- Trình biên dịch C hỗ trợ Win32, ví dụ MinGW-w64 GCC (có thể cài qua MSYS2).

### Biên dịch và chạy

Mở terminal MinGW-w64, chuyển đến thư mục `ATBM/A51` rồi chạy:

```bash
gcc -Wall -O2 -mwindows A51_gui.c -o A51.exe -lcomdlg32 -lole32 -lgdiplus -lgdi32 -luser32
```

Sau khi biên dịch thành công, chạy `A51.exe`:

```bash
./A51.exe
```

Nếu dùng Command Prompt hoặc PowerShell, có thể chạy:

```powershell
.\A51.exe
```

### Sử dụng

1. Nhập khóa **K** theo một trong hai định dạng: 16 ký tự hex (ví dụ `123456789ABCDEF0`) hoặc 64 bit `0`/`1`.
2. Nhập bản rõ vào ô dữ liệu, hoặc chọn **Chèn tệp** để dùng nội dung từ tệp.
3. Chọn **Mã hóa** hoặc **Giải mã**. Khi giải mã dữ liệu nhập trực tiếp, chọn dạng **Hex** hoặc **Nhị phân**; cũng có thể chèn tệp bản mã `.a51`.
4. Xem kết quả trên giao diện; chọn **Lưu kết quả...** để ghi ra tệp.

## TinyA51

Mã nguồn TinyA51 nằm tại `ATBM/TinyA51/tiny_a51.c`. Có thể mở và chạy trực tiếp bằng IDE C đang sử dụng; chương trình tương tác qua console, nhận khóa 23 bit và dữ liệu dạng bit (tối đa 100 bit), đồng thời in các bước quay thanh ghi.

> **Lưu ý:** Đây là mã nguồn phục vụ học tập và minh họa thuật toán, không nên dùng để bảo vệ dữ liệu thực tế. A5/1 là thuật toán cũ và không còn an toàn cho mục đích bảo mật hiện đại.
