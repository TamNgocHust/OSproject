#ifndef OS_KERNEL_H
#define OS_KERNEL_H

#include <stdint.h>

/* Cấu trúc khối quản lý tiến trình (TCB - Task Control Block) */
typedef struct {
    /* ĐIỀU KIỆN BẮT BUỘC: Biến lưu con trỏ Stack (sp) phải nằm ở vị trí ĐẦU TIÊN của struct.
     * Để sau này code Assembly dễ dàng lấy ra được. */
    uint32_t *sp;       
    
    /* Các thông tin phụ */
    uint32_t id;        /* ID của Task */
} TCB_t;

/* Khai báo các hàm hệ điều hành */
void OS_KernelInit(void);
void OS_TaskCreate(TCB_t *tcb, void (*task_func)(void), uint32_t *stack, uint32_t stack_size, uint32_t id);

/* Khai báo 2 con trỏ toàn cục để Assembly (PendSV) có thể nhìn thấy và tráo đổi */
extern TCB_t *os_current_tcb;
extern TCB_t *os_next_tcb;

/* Hàm khởi động hệ điều hành */
void OS_Start(TCB_t *task1, TCB_t *task2);
#endif