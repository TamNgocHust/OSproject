#include <stdint.h>
#include <stddef.h>
#include "os_kernel.h"

extern void UART_Init(void);
extern void UART_Print(const char* str);
/* KHAI BÁO SỬ DỤNG HÀM CÓ SẴN TỪ DRIVER UART */
extern void UART_PrintNum(uint32_t num, uint8_t digits); 

#define STACK_SIZE_EMERG 128
#define STACK_SIZE_MEM   128
#define STACK_SIZE_WORK  64

uint32_t stack_emerg[STACK_SIZE_EMERG];
uint32_t stack_mem[STACK_SIZE_MEM];
uint32_t stack_wkA[STACK_SIZE_WORK];
uint32_t stack_wkB[STACK_SIZE_WORK];

os_mutex_t uart_mutex;
uint64_t start_ms = 0; 

/* =========================================================================
 * HÀM PHỤ TRỢ: IN ĐỊNH DẠNG THỜI GIAN THEO REAL-TIME
 * ========================================================================= */
void UART_Print_With_Time(const char* task_name, const char* str) {
    uint32_t elapsed_ms = os_get_tick(); 
    uint64_t total_ms = start_ms + elapsed_ms;
    
    uint32_t ms = total_ms % 1000;
    uint64_t total_secs = total_ms / 1000;
    
    uint32_t secs = total_secs % 60;
    uint64_t total_mins = total_secs / 60;
    
    uint32_t mins = total_mins % 60;
    uint32_t hours = (total_mins / 60) % 24;

    /* Tự ghép chuỗi siêu nhẹ bằng cách gọi trực tiếp Driver của hệ thống */
    UART_Print("[");
    UART_PrintNum(hours, 2); UART_Print(":");
    UART_PrintNum(mins, 2);  UART_Print(":");
    UART_PrintNum(secs, 2);  UART_Print(".");
    UART_PrintNum(ms, 3);
    UART_Print("] [");
    UART_Print(task_name);
    UART_Print("] ");
    UART_Print(str);
}

/* =========================================================================
 * CÁC TÁC VỤ (TASKS)
 * ========================================================================= */
void Task_Emergency(void) {
    while (1) {
        os_delay(4000); 
        os_mutex_take(&uart_mutex);
        UART_Print_With_Time("EMERGENCY", "ALARM: CANH BAO KHAN CAP! HE THONG BI XAM NHAP!\n\n");
        os_mutex_give(&uart_mutex);
    }
}

void Task_Memory(void) {
    while (1) {
        os_mutex_take(&uart_mutex);
        UART_Print_With_Time("Task Mem", "Dang xu ly dong bo hoa va cap phat RAM...\n");
        os_mutex_give(&uart_mutex);
        
        void* ptr = os_malloc(128);
        if (ptr != NULL) {
            for(volatile int i = 0; i < 2000; i++); 
            os_free(ptr); 
        }
        os_delay(1500); 
    }
}

void Task_WorkerA(void) {
    while (1) {
        os_mutex_take(&uart_mutex);
        UART_Print_With_Time("Worker A", "Hello World!\n");
        os_mutex_give(&uart_mutex);
        os_delay(1000); 
    }
}

void Task_WorkerB(void) {
    while (1) {
        os_mutex_take(&uart_mutex);
        UART_Print_With_Time("Worker B", "He Dieu Hanh 2025.2\n");
        os_mutex_give(&uart_mutex);
        os_delay(1000); 
    }
}

/* =========================================================================
 * MAIN PROGRAM
 * ========================================================================= */
int main(void) {
    UART_Init();
    
    /* Tự động lấy giờ của máy tính lúc gõ lệnh "make" */
    const char compile_time[] = __TIME__; 
    uint32_t start_hour = (compile_time[0]-'0')*10 + (compile_time[1]-'0');
    uint32_t start_min  = (compile_time[3]-'0')*10 + (compile_time[4]-'0');
    uint32_t start_sec  = (compile_time[6]-'0')*10 + (compile_time[7]-'0');
    
    start_ms = ((uint64_t)start_hour * 3600 + (uint64_t)start_min * 60 + start_sec) * 1000;

    UART_Print("\n=== KHOI DONG RTOS THEO GIO THUC TE (AUTO-SYNC) ===\n");

    os_init();
    os_mutex_init(&uart_mutex);

    os_create_task(Task_WorkerA, stack_wkA, STACK_SIZE_WORK, 1);
    os_create_task(Task_WorkerB, stack_wkB, STACK_SIZE_WORK, 1);
    os_create_task(Task_Memory,  stack_mem, STACK_SIZE_MEM,  2);
    os_create_task(Task_Emergency, stack_emerg, STACK_SIZE_EMERG, 3);

    os_start();

    while (1) {
        __asm volatile("wfi"); 
    }
    return 0; 
}