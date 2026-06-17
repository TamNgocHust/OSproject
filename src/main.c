#include <stdint.h>
#include "os_kernel.h"

/* Khai báo các hàm ngoại vi tự viết từ stm32f100_uart.c */
extern void UART_Init(void);
extern void UART_Print(const char* str);
extern void UART_PrintNum(uint32_t num);

/* =========================================================================
 * KHAI BÁO TÀI NGUYÊN CHO CÁC TASK
 * ========================================================================= */
#define STACK_SIZE 256  /* Mỗi Task được cấp một mảng 256 biến 32-bit (1024 bytes) làm Stack */

/* Khai báo 2 khối TCB để quản lý 2 ứng dụng */
TCB_t task1_tcb;
TCB_t task2_tcb;

/* Khai báo mảng RAM tĩnh làm Stack (Kiểm soát chặt chẽ, tuyệt đối không dùng malloc) */
uint32_t task1_stack[STACK_SIZE];
uint32_t task2_stack[STACK_SIZE];

/* =========================================================================
 * MÃ NGUỒN CỦA 2 ỨNG DỤNG (TASKS) CHẠY SONG SONG
 * ========================================================================= */
void Task1(void) {
    while (1) {
        UART_Print("Xin chao tu TASK 1\n");
        /* Trễ một chút để dễ nhìn trên QEMU */
        for(volatile int i = 0; i < 300000; i++); 
    }
}

void Task2(void) {
    while (1) {
        UART_Print("Chao dong chi tu TASK 2\n");
        for(volatile int i = 0; i < 300000; i++);
    }
}

/* =========================================================================
 * HÀM MAIN - HẠT NHÂN KHỞI ĐỘNG
 * ========================================================================= */
int main(void) {
    /* 1. Khởi tạo UART */
    UART_Init();
    UART_Print("Hardware OK. Tien hanh dong goi Stack cho cac Task...\n");

    /* 2. Nhồi dữ liệu giả lập vào Stack và liên kết với TCB */
    OS_TaskCreate(&task1_tcb, Task1, task1_stack, STACK_SIZE, 1);
    OS_TaskCreate(&task2_tcb, Task2, task2_stack, STACK_SIZE, 2);

    UART_Print("Khoi tao hoan tat! Trao quyen dieu khien cho RTOS...\n");

    /* 3. Gọi hàm khởi động Hệ điều hành, nạp 2 Task vào bệ phóng.
     * Hàm này sẽ tự bật SysTick và gọi ngắt hoán đổi PendSV.
     */
    OS_Start(&task1_tcb, &task2_tcb);

    /* 4. Vòng lặp rảnh rỗi (Idle Loop) của hệ điều hành.
     * Khi OS đã chạy, CPU sẽ chỉ nhảy vào đây nếu không có Task nào cần chạy.
     */
    while (1) {
        // Nghỉ ngơi
    }
    
    return 0; 
}