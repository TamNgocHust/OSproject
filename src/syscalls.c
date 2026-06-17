#include <sys/stat.h>
#include <unistd.h>
/* Khai báo hàm UART ở tệp khác để dùng tạm */
extern void UART_SendChar(char c);
/* =========================================================================
 * 1. KHÓA LỖI AN TOÀN (SAFE CRASH)
 * ========================================================================= */
/* Hàm này được gọi khi hệ thống gặp lỗi nghiêm trọng (crash). 
 * Ta dùng vòng lặp vô hạn để giữ nguyên trạng thái CPU phục vụ việc debug. */
void _exit(int status) {
    (void)status; // Tránh cảnh báo biến không sử dụng
    for (;;) {
        // CPU mắc kẹt tại đây
    }
}

/* =========================================================================
 * 2. VÔ HIỆU HÓA CẤP PHÁT ĐỘNG (NO DYNAMIC ALLOCATION)
 * ========================================================================= */
/* Hàm _sbrk là lõi của lệnh malloc/free. 
 * Bằng cách luôn trả về -1, ta ép hệ điều hành chỉ được dùng mảng RAM tĩnh, 
 * loại trừ nguy cơ phân mảnh và tràn bộ nhớ. */
void *_sbrk(int incr) {
    (void)incr;
    return (void *)-1; 
}

/* =========================================================================
 * 3. CẤU HÌNH LUỒNG TERMINAL
 * ========================================================================= */
/* Báo cho hệ thống biết luồng xuất dữ liệu là một Terminal ảo */
int _isatty(int file) {
    /* Trả về 1 nếu file descriptor là luồng chuẩn (stdin, stdout, stderr) */
    if (file >= 0 && file <= 2)
        return 1;
    return 0;
}

int _fstat(int file, struct stat *st) {
    (void)file;
    /* Gán thuộc tính S_IFCHR để xác định đây là thiết bị ký tự (Character Device) */
    st->st_mode = S_IFCHR;
    return 0;
}

/* =========================================================================
 * 4. CÁC HÀM RỖNG ĐỂ TRÁNH LỖI BIÊN DỊCH
 * ========================================================================= */
int _close(int file) { return -1; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _read(int file, char *ptr, int len) { return 0; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { return -1; }

/* Lưu ý đặc biệt: Ở Giai đoạn 3, ta sẽ sửa lại hàm _write này 
 * để nối nó với Driver UART, giúp lệnh printf in được ra màn hình */
/* Nối luồng dữ liệu chuẩn (stdout) vào UART để dùng được printf */
int _write(int file, char *ptr, int len) {
    /* Chỉ in ra nếu là luồng xuất tiêu chuẩn (stdout = 1 hoặc stderr = 2) */
    if (file == 1 || file == 2) {
        for (int i = 0; i < len; i++) {
            UART_SendChar(ptr[i]);
        }
        return len;
    }
    return -1;
}