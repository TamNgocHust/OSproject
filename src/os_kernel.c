#include "os_kernel.h"
#include <stddef.h>

volatile uint32_t os_tick_count = 0;

TCB_t *os_current_tcb = NULL;
TCB_t *os_next_tcb = NULL;
TCB_t *os_tasks[2];
uint32_t os_current_task_idx = 0;

void OS_KernelInit(void) {
    uint32_t ticks = 8000;
    *((volatile uint32_t *)0xE000E014) = (ticks - 1);
    *((volatile uint32_t *)0xE000E018) = 0;
    *((volatile uint32_t *)0xE000E010) = (1<<2) | (1<<1) | (1<<0);
}

void OS_TaskCreate(TCB_t *tcb, void (*task_func)(void), uint32_t *stack, uint32_t stack_size, uint32_t id) {
    tcb->id = id;
    uint32_t *sp = &stack[stack_size];
    
    /* Hardware Frame (CPU tự PUSH khi có ngắt) */
    *(--sp) = 0x01000000;         // xPSR (Trạng thái)
    *(--sp) = (uint32_t)task_func;// PC (Trỏ tới hàm)
    *(--sp) = 0xFFFFFFFD;         // LR 
    *(--sp) = 0x12121212;         // R12
    *(--sp) = 0x03030303;         // R3
    *(--sp) = 0x02020202;         // R2
    *(--sp) = 0x01010101;         // R1
    *(--sp) = 0x00000000;         // R0
    
    /* Software Frame (Tay ta tự PUSH trong PendSV) */
    *(--sp) = 0x11111111; // R11
    *(--sp) = 0x10101010; // R10
    *(--sp) = 0x09090909; // R9
    *(--sp) = 0x08080808; // R8
    *(--sp) = 0x07070707; // R7
    *(--sp) = 0x06060606; // R6
    *(--sp) = 0x05050505; // R5
    *(--sp) = 0x04040404; // R4

    tcb->sp = sp;
}

void OS_Start(TCB_t *task1, TCB_t *task2) {
    os_tasks[0] = task1;
    os_tasks[1] = task2;

    os_current_tcb = os_tasks[0];
    os_next_tcb = os_tasks[0];

    /* Đặt con trỏ Stack tiến trình (PSP) về 0 để PendSV biết đây là lần khởi động đầu tiên */
    __asm volatile ("mov r0, #0 \n msr psp, r0");

    OS_KernelInit();

    /* Bắn phát súng đầu tiên, gọi ngắt PendSV ép CPU nhảy vào Task 1 */
    *((volatile uint32_t *)0xE000ED04) = (1 << 28); 
}

void SysTick_Handler(void) {
    os_tick_count++;

    /* Thuật toán Lập lịch (Round-Robin) */
    os_current_task_idx = (os_current_task_idx + 1) % 2;
    os_next_tcb = os_tasks[os_current_task_idx];

    /* Nếu đến lượt Task khác chạy, báo động cho PendSV */
    if (os_current_tcb != os_next_tcb) {
        *((volatile uint32_t *)0xE000ED04) = (1 << 28); 
    }
}

/* =========================================================================
 * PHÉP THUẬT CHUYỂN NGỮ CẢNH (Viết bằng Inline Assembly)
 * ========================================================================= */
__attribute__((naked)) void PendSV_Handler(void) {
    __asm volatile (
        "cpsid i \n\t"                  /* 1. Tắt ngắt toàn cục để tránh bị quấy rầy */
        "mrs r0, psp \n\t"              /* 2. R0 = con trỏ Stack (PSP) của Task cũ */
        "cbz r0, Restore_Next_Task \n\t"/* 3. Nếu PSP = 0 (Lần đầu chạy OS), bỏ qua bước lưu */

        /* --- BƯỚC 1: LƯU TRẠNG THÁI TASK CŨ --- */
        "stmdb r0!, {r4-r11} \n\t"      /* 4. Nhét R4-R11 vào đỉnh Stack của Task cũ */
        "ldr r1, =os_current_tcb \n\t"  
        "ldr r2, [r1] \n\t"             
        "str r0, [r2] \n\t"             /* 5. Ghi nhớ địa chỉ Stack mới nhất vào thẻ căn cước (TCB) */

    "Restore_Next_Task: \n\t"
        /* --- BƯỚC 2: ĐỔI SANG TASK MỚI --- */
        "ldr r1, =os_current_tcb \n\t"
        "ldr r3, =os_next_tcb \n\t"
        "ldr r4, [r3] \n\t"             
        "str r4, [r1] \n\t"             /* 6. os_current_tcb = os_next_tcb */

        /* --- BƯỚC 3: PHỤC HỒI TRẠNG THÁI TASK MỚI --- */
        "ldr r0, [r4] \n\t"             /* 7. Đọc con trỏ Stack từ TCB mới */
        "ldmia r0!, {r4-r11} \n\t"      /* 8. Kéo R4-R11 của Task mới ra khỏi Stack */
        "msr psp, r0 \n\t"              /* 9. Nạp lại thanh ghi PSP cho CPU */

        /* --- BƯỚC 4: THOÁT NGẮT VÀ TIẾP TỤC CHẠY --- */
        "cpsie i \n\t"                  /* 10. Bật lại ngắt */
        "ldr lr, =0xFFFFFFFD \n\t"      /* 11. Mã trả về đặc biệt để CPU tự POP nốt PC và xPSR */
        "bx lr \n\t"                    /* 12. PÙM! CPU dịch chuyển tức thời sang ứng dụng mới! */
        : : : "memory"
    );
}