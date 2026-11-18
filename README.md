# ATBMTT

Bộ mã nguồn minh họa A5/1 và TinyA5/1 bằng ngôn ngữ C, gồm:

- Phiên bản A5/1 đầy đủ với giao diện Windows Win32.
- Phiên bản TinyA5/1 chạy trên console để quan sát từng bước sinh khóa dòng.

> Đây là chương trình phục vụ học tập và minh họa; không nên sử dụng A5/1 để bảo vệ dữ liệu thực tế.

## Cấu trúc thư mục

```text
ATBMTT/
├── README.md
└── ATBM/
    ├── A51/
    │   ├── A51_gui.c   # Giao diện và luồng mã hóa/g giải mã Win32
    │   ├── A51.h       # Các thanh ghi, khóa và hàm sinh bit khóa dòng
    │   ├── A51_util.h  # Tiện ích chuyển đổi hex/nhi phân và hiển thị trạng thái
    │   ├── mahoa.h     # Hàm mã hóa P XOR S
    │   └── giaima.h    # Hàm giải mã C XOR S
    └── TinyA51/
        └── tiny_a51.c  # Phiên bản 23-bit, in chi tiết từng bước ra console
```

## Phiên bản A5/1 đầy đủ

### Yêu cầu

- Windows.
- MinGW-w64 GCC hoặc trình biên dịch C hỗ trợ Win32.

### Biên dịch

Mở terminal MinGW-w64, chuyển tới thư mục `ATBM/A51` và chạy:

```bash
gcc -Wall -O2 -mwindows A51_gui.c -o A51.exe -lcomdlg32 -lole32 -lgdiplus -lgdi32 -luser32
```

### Chạy chương trình

MinGW/MSYS:

```bash
./A51.exe
```

Command Prompt hoặc PowerShell:

```powershell
.\A51.exe
```

### Tính năng

- Nhập khóa `K` ở dạng **16 ký tự hex** hoặc **64 bit 0/1**.
- Nhập bản rõ trực tiếp ở dạng văn bản, hex hoặc nhị phân.
- Chèn tệp văn bản, ảnh hoặc tệp nhị phân đến 200 KB.
- Mã hóa, giải mã và lưu kết quả ra tệp.
- Hiển thị kết quả bằng văn bản, mô tả nhị phân hoặc hex.
- Hiển thị trạng thái ba thanh ghi X, Y, Z sau khi xử lý.
- Xem trước ảnh bằng GDI+ khi dữ liệu đầu vào hoặc kết quả là ảnh hợp lệ.

### Sử dụng

1. Nhập khóa `K`:
   - Ví dụ hex: `123456789ABCDEF0`.
   - Hoặc nhập đủ 64 ký tự `0` và `1`.
2. Chọn dạng nhập:
   - **Văn bản**: dành cho mã hóa chuỗi UTF-8 được nhập trực tiếp.
   - **Hex** hoặc **Nhị phân**: dùng để nhập bản mã hoặc dữ liệu nhị phân.
3. Gõ dữ liệu hoặc nhấn **Chèn tệp** để nạp tệp.
4. Chọn **Mã hóa** hoặc **Giải mã**.
5. Xem kết quả và trạng thái thanh ghi.
6. Chọn **Lưu kết quả...** để ghi dữ liệu ra tệp.

Khi giải mã dữ liệu được gõ trực tiếp, cần chọn **Hex** hoặc **Nhị phân**. Đối với văn bản cần mã hóa, chỉ chọn **Văn bản**; bản giải mã văn bản được hiển thị nếu dữ liệu kết quả là UTF-8 hợp lệ.

### Các file thành phần

- `A51.h`: định nghĩa thanh ghi X/Y/Z, hàm trộn chẵn lẻ, quay thanh ghi, chia khóa và chuyển khóa hex sang chuỗi 64 bit.
- `A51_util.h`: hỗ trợ chuyển hex và nhị phân thành byte, chuyển dữ liệu thành chuỗi hex/bit và định dạng trạng thái thanh ghi.
- `mahoa.h`: nạp khóa, sinh dãy khóa dòng và thực hiện `P XOR S`.
- `giaima.h`: nạp khóa, sinh lại dãy khóa dòng và thực hiện `C XOR S`.
- `A51_gui.c`: giao diện Win32, nạp/lưu tệp, xử lý hex/nhị phân/văn bản và xem trước ảnh.

## Phiên bản TinyA5/1

Tệp nguồn `ATBM/TinyA51/tiny_a51.c` mô phỏng phiên bản rút gọn với ba thanh ghi có kích thước 6, 8 và 9 bit.

### Biên dịch

```bash
gcc -Wall -O2 tiny_a51.c -o tiny_a51
```

### Chạy

Windows:

```powershell
.\tiny_a51.exe
```

Linux/macOS:

```bash
./tiny_a51
```

### Sử dụng

Chương trình hỏi lựa chọn mã hóa hoặc giải mã, sau đó yêu cầu:

- Khóa nhị phân đúng **23 bit**.
- Chuỗi bản rõ hoặc bản mã chỉ gồm các ký tự `0/1`, tối đa 100 bit.

Mỗi bước của chương trình sẽ in:

- Trạng thái trước và sau khi quay của X, Y, Z.
- Chỉ số trộn đa số `m`.
- Thanh ghi được quay hoặc giữ nguyên.
- Bit khóa dòng `s = x5 XOR y7 XOR z8`.
- Chuỗi khóa dòng và kết quả mã hóa/g giải mã cuối cùng.

## Ghi chú

- Chuẩn A5/1 đầy đủ trong thư mục `A51` sử dụng khóa 64 bit và ba thanh ghi kích thước 19, 22, 23.
- TinyA5/1 là mô hình giáo dục 23 bit để theo dõi thuật toán từng bước, không tương thích trực tiếp với các khóa và dữ liệu của bản đầy đủ.
- Thao tác mã hóa và giải mã với các tệp lưu trong thư mục làm việc hiện tại và giao diện hỗ trợ lưu kết quả bằng hộp thoại Windows.
