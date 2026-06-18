#include <stdint.h>
#include "os_kernel.h"

/* Các hàm UART tự viết */
extern void UART_Init(void);
extern void UART_Print(const char* str);
extern void UART_PrintNum(uint32_t num);

/* =========================================================================
 * KHAI BÁO TÀI NGUYÊN
 * ========================================================================= */
#define STACK_SIZE 256  

/* Chỉ cần cấp phát 2 mảng Stack tĩnh cho 2 Task */
uint32_t task1_stack[STACK_SIZE];
uint32_t task2_stack[STACK_SIZE];

/* Khai báo Mutex toàn cục để bảo vệ cổng UART (Đồng bộ hóa) */
os_mutex_t uart_mutex;

/* =========================================================================
 * MÃ NGUỒN CỦA 2 ỨNG DỤNG (TÍCH HỢP MUTEX)
 * ========================================================================= */
void Task1(void) {
    while (1) {
        /* Xin cấp quyền sử dụng Mutex trước khi dùng UART */
        os_mutex_take(&uart_mutex);
        
        /* -------- VÙNG CRITICAL SECTION -------- */
        UART_Print("Hello World!\n");
        
        /* Vòng lặp rỗng mô phỏng một tác vụ xử lý tốn thời gian.
         * Nhờ có Mutex, Task 2 sẽ bị khóa (Blocked) và không thể xen ngang vào đây. */
        for(volatile int i = 0; i < 10000; i++); 
        /* --------------------------------------- */
        
        /* Xong việc, trả lại Mutex để Task khác có thể dùng */
        os_mutex_give(&uart_mutex);
        
        /* Cho Task 1 ngủ 1000ms */
        os_delay(1000); 
    }
}

void Task2(void) {
    while (1) {
        /* Chờ lấy bằng được chìa khóa Mutex mới chạy tiếp */
        os_mutex_take(&uart_mutex);
        
        /* -------- VÙNG CRITICAL SECTION -------- */
        UART_Print("Day la nhom 11\n");
        
        for(volatile int i = 0; i < 10000; i++); 
        /* --------------------------------------- */
        
        /* Xong việc, nhả Mutex ra */
        os_mutex_give(&uart_mutex);
        
        /* Task 2 ngủ 1500ms */
        os_delay(1500); 
    }
}

/* =========================================================================
 * HÀM MAIN - HẠT NHÂN KHỞI ĐỘNG
 * ========================================================================= */
int main(void) {
    UART_Init();
    UART_Print("Hardware OK. Khoi tao kien truc RTOS cua Nhom 11...\n");

    /* 1. Khởi tạo dữ liệu hệ điều hành */
    os_init();
    
    /* 2. Khởi tạo Mutex bảo vệ UART */
    os_mutex_init(&uart_mutex);

    /* 3. Đăng ký Task vào hệ thống */
    os_create_task(Task1, task1_stack, STACK_SIZE);
    os_create_task(Task2, task2_stack, STACK_SIZE);

    UART_Print("Bat dau chay Scheduler...\n");

    /* 4. Khởi động OS */
    os_start();

    /* 5. CPU sẽ chỉ lọt vào đây nếu tất cả các Task đều đang ngủ (IDLE) */
    while (1) {
        /* Chờ ngắt (Đưa phần cứng vào chế độ tiết kiệm điện) */
        __asm volatile("wfi"); 
    }
    
    return 0; 
}