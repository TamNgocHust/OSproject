#include "stm32f100.h"

/* Khởi tạo UART1 ở tốc độ 115200 baud (Giả sử xung nhịp hệ thống mặc định là 8MHz) */
void UART_Init(void) {
    /* 1. Bật xung nhịp cho PORT A và USART1 */
    /* Bit 2: IOPAEN, Bit 14: USART1EN trên thanh ghi APB2ENR */
    RCC->APB2ENR |= (1 << 2) | (1 << 14);

    /* 2. Cấu hình chân PA9 làm chân truyền (TX) - Chế độ Alternate Function Push-Pull */
    /* PA9 thuộc thanh ghi CRH (chân 8 đến 15). Xóa cấu hình cũ của PA9 (bit 4->7) */
    GPIOA->CRH &= ~(0xF << 4);
    /* Set PA9 thành Output 50MHz (11) và Push-Pull Alternate (10) => Mã Hex là 0xB */
    GPIOA->CRH |= (0xB << 4);

    /* 3. Cấu hình tốc độ Baudrate = 115200 */
    /* Công thức: 8.000.000 / (16 * 115200) = 4.34. Phần nguyên = 4, Phần thập phân = 0.34 * 16 = 5 => Hex = 0x45 */
    USART1->BRR = 0x45;

    /* 4. Bật UART1 (UE) và bật bộ truyền (TE) trên thanh ghi CR1 */
    /* Bit 13: UE (USART Enable), Bit 3: TE (Transmitter Enable) */
    USART1->CR1 = (1 << 13) | (1 << 3);
}

/* Hàm gửi một ký tự qua UART */
// void UART_SendChar(char c) {
//     /* Đợi cho đến khi thanh ghi dữ liệu trống (Cờ TXE = 1 ở bit 7) */
//     while (!(USART1->SR & (1 << 7))) {
//         // Chờ đợi
//     }
//     /* Ghi ký tự vào thanh ghi DR để phần cứng tự động phát đi */
//     USART1->DR = (c & 0xFF);
// }
/* HACK: Hàm mới dành cho máy ảo QEMU (Board lm3s6965evb) */
void UART_SendChar(char c) {
    /* Trong mạch lm3s6965evb của QEMU, cổng UART0 nằm ở địa chỉ 0x4000C000.
     * QEMU sẽ ngay lập tức in ra terminal bất kỳ ký tự nào được ghi vào đây. */
    volatile uint32_t *qemu_uart_dr = (uint32_t *)0x4000C000;
    *qemu_uart_dr = c;
}
/* Hàm in chuỗi ký tự siêu nhẹ (Bỏ qua thư viện C) */
void UART_Print(const char* str) {
    while (*str != '\0') {
        UART_SendChar(*str);
        str++;
    }
}

/* Hàm in số nguyên tự viết */
void UART_PrintNum(uint32_t num) {
    if (num == 0) {
        UART_SendChar('0');
        return;
    }
    char buf[10];
    int i = 0;
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    /* In ngược lại từ mảng ra màn hình */
    while (i > 0) {
        UART_SendChar(buf[--i]);
    }
}