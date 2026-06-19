#include <stdint.h>
#include <stddef.h>  /* ĐÃ BỔ SUNG: Thư viện chứa từ khóa NULL để sửa lỗi */
#include "os_kernel.h"

extern void UART_Init(void);
extern void UART_Print(const char* str);

#define STACK_SIZE 128 
uint32_t stack_emerg[STACK_SIZE];
uint32_t stack_mem[STACK_SIZE];
uint32_t stack_wkA[STACK_SIZE];
uint32_t stack_wkB[STACK_SIZE];

os_mutex_t uart_mutex;

/* =========================================================================
 * 1. TASK ĐẶC QUYỀN (MỨC 3) - CẢNH BÁO KHẨN CẤP
 * ========================================================================= */
void Task_Emergency(void) {
    while (1) {
        os_delay(4000); /* Ngủ đông 4 giây */
        
        /* Ngay khi tỉnh giấc ở giây thứ 4, do có mức ưu tiên cao nhất,
         * OS sẽ ĐÁ VĂNG mọi Task khác đang chạy để Task này được cảnh báo */
        os_mutex_take(&uart_mutex);
        UART_Print("\n[!] ALARM: CANH BAO KHAN CAP! HE THONG BI XAM NHAP!\n\n");
        os_mutex_give(&uart_mutex);
    }
}

/* =========================================================================
 * 2. TASK TẦM TRUNG (MỨC 2) - XỬ LÝ BỘ NHỚ BUDDY SYSTEM
 * ========================================================================= */
void Task_Memory(void) {
    while (1) {
        os_mutex_take(&uart_mutex);
        UART_Print("[Task Mem] Dang xu ly dong bo hoa va cap phat RAM...\n");
        os_mutex_give(&uart_mutex);
        
        /* Thử nghiệm thuật toán Buddy System (Xin 128 byte RAM) */
        void* ptr = os_malloc(128);
        if (ptr != NULL) {
            /* Giả lập xử lý dữ liệu phức tạp */
            for(volatile int i = 0; i < 2000; i++); 
            os_free(ptr); /* Dùng xong trả ngay */
        }
        
        os_delay(1500); 
    }
}

/* =========================================================================
 * 3. TASK DÂN ĐEN (MỨC 1) - TÍNH TOÁN NỀN
 * ========================================================================= */
void Task_WorkerA(void) {
    while (1) {
        os_mutex_take(&uart_mutex);
        UART_Print("Hello World!\n");
        os_mutex_give(&uart_mutex);
        os_delay(1000); 
    }
}

void Task_WorkerB(void) {
    while (1) {
        os_mutex_take(&uart_mutex);
        UART_Print("He Dieu Hanh 2025.2\n");
        os_mutex_give(&uart_mutex);
        os_delay(1000); 
    }
}

/* =========================================================================
 * MAIN PROGRAM
 * ========================================================================= */
int main(void) {
    UART_Init();
    UART_Print("\nBat Dau\n");

    os_init();
    os_mutex_init(&uart_mutex);

    /* Tham số thứ 4 chính là ĐỘ ƯU TIÊN (Priority) */
    os_create_task(Task_WorkerA, stack_wkA, STACK_SIZE, 1);
    os_create_task(Task_WorkerB, stack_wkB, STACK_SIZE, 1);
    os_create_task(Task_Memory,  stack_mem, STACK_SIZE, 2);
    os_create_task(Task_Emergency, stack_emerg, STACK_SIZE, 3);

    os_start();

    while (1) {
        __asm volatile("wfi"); 
    }
    return 0; 
}