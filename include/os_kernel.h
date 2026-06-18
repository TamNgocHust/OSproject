#ifndef OS_KERNEL_H
#define OS_KERNEL_H

#include <stdint.h>

/* =========================================================================
 * CẤU HÌNH HỆ THỐNG VÀ TRẠNG THÁI TÁC VỤ
 * ========================================================================= */

/* Giới hạn số lượng tác vụ tối đa trong hệ thống là 4 Tasks */
#define OS_MAX_TASKS 4

/* Định nghĩa trạng thái hoạt động của Tác vụ */
/* Định nghĩa trạng thái hoạt động của Tác vụ */
typedef enum {
    OS_TASK_UNUSED = 0, 
    OS_TASK_READY,      
    OS_TASK_RUNNING,    
    OS_TASK_DELAYED,
    OS_TASK_BLOCKED     /* TRẠNG THÁI MỚI: Tác vụ đang chờ Mutex/Semaphore */
} os_task_state_t;

/* =========================================================================
 * KHỐI ĐỒNG BỘ HÓA - MUTEX
 * ========================================================================= */
typedef struct {
    uint8_t is_locked;      /* 0: Đang rảnh, 1: Đã bị khóa */
    uint32_t owner_task_id; /* Lưu ID của Task đang giữ Mutex */
} os_mutex_t;

/* Các hàm API cho Mutex */
void os_mutex_init(os_mutex_t *mutex);
void os_mutex_take(os_mutex_t *mutex);
void os_mutex_give(os_mutex_t *mutex);
/* =========================================================================
 * KHỐI QUẢN LÝ TÁC VỤ (TASK CONTROL BLOCK)
 * ========================================================================= */
typedef struct {
    /* ĐIỀU KIỆN BẮT BUỘC: Biến sp phải nằm ở vị trí ĐẦU TIÊN của struct */
    uint32_t *sp;             /* Con trỏ Ngăn xếp (Stack Pointer) */
    
    os_task_state_t state;    /* Trạng thái hoạt động hiện tại của Task */
    uint32_t delay;           /* Bộ đếm thời gian lùi (ms) khi gọi hàm trì hoãn */
} os_tcb_t;


/* =========================================================================
 * CÁC HÀM GIAO TIẾP LẬP TRÌNH HỆ THỐNG (APIs)
 * ========================================================================= */

/* --- 1. Điều khiển vòng đời Task --- */
void os_init(void);
void os_create_task(void (*task_func)(void), uint32_t *stack, uint32_t stack_size);
void os_start(void);

/* --- 2. Điều phối thời gian thực --- */
void os_yield(void);
void os_delay(uint32_t ms);

/* --- 3. Quản lý Nhịp hệ thống & Lập lịch --- */
void os_tick_handler(void);
uint32_t os_get_tick(void);
uint32_t *os_schedule(uint32_t *current_sp);
/* ---  Cơ chế bẫy lỗi hệ thống --- */
void os_task_exit_handler(void);
#endif /* OS_KERNEL_H */