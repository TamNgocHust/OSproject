#include "os_kernel.h"
#include <stddef.h>

/* Khai báo hàm Driver ngoại vi từ tệp khác để phục vụ in log bẫy lỗi */
extern void UART_Print(const char* str);

/* =========================================================================
 * KHAI BÁO BIẾN TOÀN CỤC HỆ THỐNG
 * ========================================================================= */
os_tcb_t os_task_table[OS_MAX_TASKS];
uint32_t os_task_count = 0;
uint32_t os_current_task_idx = 0;

volatile uint32_t os_system_tick = 0;

/* =========================================================================
 * IDLE TASK (TIẾN TRÌNH RẢNH RỖI CỦA HỆ ĐIỀU HÀNH)
 * ========================================================================= */
uint32_t os_idle_stack[128];
void os_idle_task(void) {
    while (1) {
        /* Lệnh WFI đưa vi điều khiển vào chế độ tiết kiệm điện.
         * CPU sẽ dừng hoạt động cho đến khi có ngắt SysTick (mỗi 1ms) đánh thức nó. */
        __asm volatile("wfi"); 
    }
}

/* =========================================================================
 * 1. ĐIỀU KHIỂN VÒNG ĐỜI TASK
 * ========================================================================= */
void os_init(void) {
    /* Khởi tạo hệ thống quản lý bộ nhớ Buddy System trước tiên */
    os_buddy_init();

    os_task_count = 0;
    os_current_task_idx = 0;
    for (int i = 0; i < OS_MAX_TASKS; i++) {
        os_task_table[i].state = OS_TASK_UNUSED;
        os_task_table[i].delay = 0;
    }
    
    /* OS luôn tự động tạo Idle Task ở vị trí số 0 với độ ưu tiên 0 (Thấp nhất) */
    os_create_task(os_idle_task, os_idle_stack, 128, 0);
}

/* ĐÃ CẬP NHẬT: Thêm tham số uint8_t priority vào hàm */
void os_create_task(void (*task_func)(void), uint32_t *stack, uint32_t stack_size, uint8_t priority) {
    if (os_task_count >= OS_MAX_TASKS) return; 

    /* Lấy địa chỉ vùng nhớ đỉnh Stack ban đầu do người dùng cấp */
    uint32_t stack_top = (uint32_t)&stack[stack_size];
    
    /* Ép đỉnh Ngăn xếp căn chỉnh 8-byte theo chuẩn ARM EABI AAPCS */
    uint32_t *sp = (uint32_t *)(stack_top & ~0x7UL);

    /* 1. Giả lập khung ngắt do PHẦN CỨNG tự động Push khi xảy ra ngắt (8 thanh ghi) */
    *(--sp) = 0x01000000;           /* xPSR: Bật bit T (Thumb mode) để tránh lỗi HardFault */
    *(--sp) = (uint32_t)task_func;  /* PC (Program Counter): Trỏ vào hàm thực thi của Task */
    
    /* Gài hàm bẫy lỗi vào LR thay vì gán giá trị tĩnh rác 0xFFFFFFFD.
     * Nếu tác vụ chạy hết lệnh và return, CPU sẽ tự động nhảy vào hàm os_task_exit_handler */
    *(--sp) = (uint32_t)os_task_exit_handler; 
    
    *(--sp) = 0x12121212;           /* R12 */
    *(--sp) = 0x03030303;           /* R3 */
    *(--sp) = 0x02020202;           /* R2 */
    *(--sp) = 0x01010101;           /* R1 */
    *(--sp) = 0x00000000;           /* R0 */

    /* 2. Giả lập khung ngắt bổ sung do PHẦN MỀM (Hợp ngữ PendSV) thực hiện Push (8 thanh ghi) */
    *(--sp) = 0x11111111;           /* R11 */
    *(--sp) = 0x10101010;           /* R10 */
    *(--sp) = 0x09090909;           /* R9 */
    *(--sp) = 0x08080808;           /* R8 */
    *(--sp) = 0x07070707;           /* R7 */
    *(--sp) = 0x06060606;           /* R6 */
    *(--sp) = 0x05050505;           /* R5 */
    *(--sp) = 0x04040404;           /* R4 */

    /* Cập nhật thông tin vào Khối quản lý tác vụ TCB */
    os_task_table[os_task_count].sp = sp;
    os_task_table[os_task_count].state = OS_TASK_READY;
    os_task_table[os_task_count].delay = 0;
    
    /* ĐÃ CẬP NHẬT: Ghi nhận mức độ ưu tiên của Task */
    os_task_table[os_task_count].priority = priority; 
    
    os_task_count++;
}

void os_start(void) {
    if (os_task_count == 0) return;
    
    /* Reset con trỏ PSP về 0 để báo hiệu lần đầu chạy hệ thống */
    __asm volatile ("mov r0, #0 \n msr psp, r0");

    /* Cấu hình bộ định thời SysTick phát ngắt định kỳ 1ms */
    uint32_t ticks = 8000;
    *((volatile uint32_t *)0xE000E014) = (ticks - 1);
    *((volatile uint32_t *)0xE000E018) = 0;
    *((volatile uint32_t *)0xE000E010) = (1<<2) | (1<<1) | (1<<0);

    /* Yêu cầu chuyển ngữ cảnh để kích hoạt Task ứng dụng đầu tiên */
    os_yield();
}

/* =========================================================================
 * 2. ĐIỀU PHỐI THỜI GIAN THỰC
 * ========================================================================= */
void os_yield(void) {
    /* Set bit 28 (PENDSVSET) trên thanh ghi ICSR để kích hoạt ngắt phần mềm PendSV */
    *((volatile uint32_t *)0xE000ED04) = (1 << 28); 
}

void os_delay(uint32_t ms) {
    if (ms > 0) {
        os_task_table[os_current_task_idx].delay = ms;
        os_task_table[os_current_task_idx].state = OS_TASK_DELAYED;
        os_yield();
    }
}

/* =========================================================================
 * 3. QUẢN LÝ NHỊP HỆ THỐNG & LẬP LỊCH
 * ========================================================================= */
void os_tick_handler(void) {
    os_system_tick++;
    
    /* Quét toàn bộ danh sách để cập nhật các Task đang ngủ */
    for (uint32_t i = 0; i < os_task_count; i++) {
        if (os_task_table[i].state == OS_TASK_DELAYED) {
            os_task_table[i].delay--;
            if (os_task_table[i].delay == 0) {
                os_task_table[i].state = OS_TASK_READY;
            }
        }
    }
    os_yield();
}

uint32_t os_get_tick(void) {
    return os_system_tick;
}

void SysTick_Handler(void) {
    os_tick_handler();
}

/*  THUẬT TOÁN LẬP LỊCH DỰA TRÊN ĐỘ ƯU TIÊN */
uint32_t *os_schedule(uint32_t *current_sp) {
    /* 1. Lưu lại Stack của Task cũ */
    if (current_sp != NULL) {
        os_task_table[os_current_task_idx].sp = current_sp;
        if (os_task_table[os_current_task_idx].state == OS_TASK_RUNNING) {
            os_task_table[os_current_task_idx].state = OS_TASK_READY;
        }
    }

    uint8_t max_priority = 0;
    uint32_t next_task_idx = 0; /* Mặc định rơi vào Idle Task (idx = 0) */

    /* 2. BƯỚC LỌC 1: Quét tìm mức Priority CAO NHẤT trong số các Task đang READY */
    for (uint32_t i = 1; i < os_task_count; i++) {
        if (os_task_table[i].state == OS_TASK_READY) {
            if (os_task_table[i].priority > max_priority) {
                max_priority = os_task_table[i].priority;
            }
        }
    }

    /* 3. BƯỚC LỌC 2: Chọn Task để chạy (Kế thừa Round-Robin cho các Task CÙNG MỨC ưu tiên) */
    if (max_priority > 0) {
        for (uint32_t i = 1; i <= os_task_count; i++) {
            uint32_t idx = (os_current_task_idx + i) % os_task_count;
            if (os_task_table[idx].state == OS_TASK_READY && os_task_table[idx].priority == max_priority) {
                next_task_idx = idx;
                break;
            }
        }
    }

    /* 4. Cấp quyền CPU */
    os_current_task_idx = next_task_idx;
    os_task_table[os_current_task_idx].state = OS_TASK_RUNNING;
    return os_task_table[os_current_task_idx].sp;
}

/*
 * PHÉP THUẬT CHUYỂN NGỮ CẢNH (CONTEXT SWITCHING ASSEMBLY)
 */
__attribute__((naked)) void PendSV_Handler(void) {
    __asm volatile (
        "cpsid i \n\t"                  /* Khóa ngắt hệ thống để bảo vệ ngữ cảnh */
        "mrs r0, psp \n\t"              /* Đọc con trỏ Stack của luồng (PSP) vào R0 */
        "cbz r0, Call_Scheduler \n\t"   /* Nếu PSP = 0 (Lần đầu khởi động OS), nhảy thẳng qua phần lưu */

        "stmdb r0!, {r4-r11} \n\t"      /* Push thủ công các thanh ghi R4-R11 vào ngăn xếp tiến trình */
        
    "Call_Scheduler: \n\t"
        "bl os_schedule \n\t"           /* Gọi hàm C để tính toán bộ lập lịch. Kết quả Đỉnh Stack mới lưu ở R0 */

        "ldmia r0!, {r4-r11} \n\t"      /* Bung (Pop) các thanh ghi R4-R11 từ Stack của Task mới ra CPU */
        "msr psp, r0 \n\t"              /* Cập nhật giá trị đỉnh Stack mới này vào thanh ghi ngoại vi PSP */
        "cpsie i \n\t"                  /* Mở lại ngắt toàn cục */
        "ldr lr, =0xFFFFFFFD \n\t"      /* Ép giá trị EXC_RETURN trở về luồng ứng dụng sử dụng PSP */
        "bx lr \n\t"                    /* Thoát ngắt phần cứng, CPU tự phục hồi R0-R3, R12, LR, PC, xPSR */
        : : : "memory"
    );
}

/* 
 * 4. CƠ CHẾ ĐỒNG BỘ HÓA TIẾN TRÌNH (MUTEX API)
 */
void os_mutex_init(os_mutex_t *mutex) {
    mutex->is_locked = 0;
    mutex->owner_task_id = 0xFFFFFFFF; 
}

void os_mutex_take(os_mutex_t *mutex) {
    while (1) {
        /* Tạo vùng miền găng cấp thấp bằng cách khóa ngắt phần cứng */
        __asm volatile ("cpsid i"); 
        
        if (mutex->is_locked == 0) {
            /* Trạng thái rảnh -> Chiếm giữ tài nguyên */
            mutex->is_locked = 1;
            mutex->owner_task_id = os_current_task_idx;
            
            __asm volatile ("cpsie i"); 
            return; 
        }
        
        /* Tài nguyên đã bị khóa -> Ép Task hiện tại rơi vào trạng thái chặn (BLOCKED) */
        os_task_table[os_current_task_idx].state = OS_TASK_BLOCKED;
        __asm volatile ("cpsie i"); 
        
        /* Thực hiện nhường CPU ngay lập tức */
        os_yield(); 
    }
}

void os_mutex_give(os_mutex_t *mutex) {
    __asm volatile ("cpsid i");
    
    /* Xác thực quyền sở hữu độc quyền của Mutex */
    if (mutex->owner_task_id == os_current_task_idx) {
        mutex->is_locked = 0;
        mutex->owner_task_id = 0xFFFFFFFF;
        
        /* Đánh thức tất cả các Task ứng dụng (bỏ qua Idle Task idx 0) đang bị chặn */
        for (uint32_t i = 1; i < os_task_count; i++) {
            if (os_task_table[i].state == OS_TASK_BLOCKED) {
                os_task_table[i].state = OS_TASK_READY;
            }
        }
    }
    
    __asm volatile ("cpsie i");
    os_yield(); 
}

/* =========================================================================
 * 5. HÀM BẪY LỖI KHI TASK THOÁT SAI QUY CÁCH (TASK TERMINATION TRAP)
 * ========================================================================= */
void os_task_exit_handler(void) {
    __asm volatile ("cpsid i");
    
    UART_Print("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    UART_Print("[KERNEL WARNING]: Mot Task da tu dong thoat khoi vong lap!\n");
    UART_Print("He dieu hanh dang tien hanh co lap va huy tac vu loi nay...\n");
    UART_Print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    
    /* Chuyển sang UNUSED để Bộ lập lịch loại bỏ vĩnh viễn khỏi hàng điều phối */
    os_task_table[os_current_task_idx].state = OS_TASK_UNUSED;
    
    __asm volatile ("cpsie i");
    
    /* Vòng lặp vô hạn cưỡng bức để giữ luồng chết an toàn, liên tục yield sang tác vụ khác */
    while (1) {
        os_yield(); 
    }
}

/* =========================================================================
 * 6. QUẢN LÝ BỘ NHỚ BẰNG CÂY NHỊ PHÂN (BUDDY SYSTEM)
 * ========================================================================= */

/* Cấu trúc Header (Gắn ở đầu mỗi khối RAM được cấp phát)
 * Kích thước struct này đúng bằng 8 Byte trên kiến trúc 32-bit (đã tính Padding) */
typedef struct os_block {
    uint8_t order;          /* Cấp độ của khối (ví dụ: 5 tức là 32 byte) */
    uint8_t is_free;        /* 1: Đang rảnh, 0: Đã bị Task chiếm */
    struct os_block *next;  /* Con trỏ nối danh sách liên kết */
} os_block_t;

/* Hồ chứa RAM 4KB (Ép căn chỉnh 8-byte theo chuẩn ARM Cortex-M3) */
uint8_t os_heap[OS_POOL_SIZE] __attribute__((aligned(8)));

/* Mảng chứa 8 danh sách liên kết tương ứng với 8 cấp độ kích thước từ 32B đến 4096B */
os_block_t *free_lists[OS_NUM_ORDERS];

void os_buddy_init(void) {
    for (int i = 0; i < OS_NUM_ORDERS; i++) {
        free_lists[i] = NULL;
    }
    
    /* Ban đầu, nguyên khối 4KB là một khối duy nhất rảnh rỗi ở cấp lớn nhất */
    os_block_t *first_block = (os_block_t *)os_heap;
    first_block->order = OS_MAX_ORDER;
    first_block->is_free = 1;
    first_block->next = NULL;
    
    free_lists[OS_NUM_ORDERS - 1] = first_block;
}

void *os_malloc(uint32_t size) {
    /* Tổng size = Dữ liệu người dùng + 8 Byte Header */
    uint32_t total_size = size + sizeof(os_block_t);
    uint8_t target_order = OS_MIN_ORDER;
    
    /* Tìm cấp độ (power of 2) nhỏ nhất vừa vặn với total_size */
    while ((1UL << target_order) < total_size && target_order <= OS_MAX_ORDER) {
        target_order++;
    }
    
    if (target_order > OS_MAX_ORDER) return NULL; /* Tràn RAM */
    
    uint8_t target_idx = target_order - OS_MIN_ORDER;
    
    __asm volatile ("cpsid i"); /* Bắt đầu Vùng miền găng */
    
    /* Quét tìm một khối rảnh từ cấp độ target trở lên */
    int i;
    for (i = target_idx; i < OS_NUM_ORDERS; i++) {
        if (free_lists[i] != NULL) break;
    }
    
    if (i == OS_NUM_ORDERS) {
        __asm volatile ("cpsie i");
        return NULL; /* Không còn RAM trống nào đủ lớn */
    }
    
    /* Bốc khối RAM đó ra khỏi danh sách rảnh */
    os_block_t *block = free_lists[i];
    free_lists[i] = block->next;
    
    /* Bước bẻ đôi (Split): Chặt khối lớn thành 2 nửa cho đến khi bằng target_order */
    while (i > target_idx) {
        i--;
        uint32_t half_size = (1UL << (i + OS_MIN_ORDER));
        
        /* Tạo ra "người bạn" (Buddy) ở nửa sau của khối RAM */
        os_block_t *buddy = (os_block_t *)((uint8_t *)block + half_size);
        buddy->order = i + OS_MIN_ORDER;
        buddy->is_free = 1;
        
        /* Cất Buddy vào danh sách rảnh ở cấp độ thấp hơn */
        buddy->next = free_lists[i];
        free_lists[i] = buddy;
        
        /* Giảm cấp độ của khối ban đầu xuống 1 bậc */
        block->order = i + OS_MIN_ORDER;
    }
    
    block->is_free = 0; /* Đánh dấu đã chiếm dụng */
    __asm volatile ("cpsie i");
    
    /* Ẩn Header đi, chỉ trả về địa chỉ phần dữ liệu cho Task sử dụng */
    return (void *)((uint8_t *)block + sizeof(os_block_t));
}

void os_free(void *ptr) {
    if (!ptr) return;
    
    /* Lùi con trỏ lại 8 byte để đọc thông tin Header hệ thống */
    os_block_t *block = (os_block_t *)((uint8_t *)ptr - sizeof(os_block_t));
    
    __asm volatile ("cpsid i");
    block->is_free = 1;
    
    /* VÒNG LẶP GỘP RAM (MERGE BUDDY) */
    while (block->order < OS_MAX_ORDER) {
        /* Phép thuật Bitwise: Tìm địa chỉ của Buddy bằng phép XOR (Tuyệt đỉnh tốc độ) */
        uint32_t offset = (uint8_t *)block - os_heap;
        uint32_t buddy_offset = offset ^ (1UL << block->order);
        os_block_t *buddy = (os_block_t *)(os_heap + buddy_offset);
        
        /* Nếu người bạn kia đang bị xài, hoặc đã bị chẻ nhỏ hơn -> Dừng gộp */
        if (!buddy->is_free || buddy->order != block->order) {
            break; 
        }
        
        /* Tìm và rút Buddy ra khỏi danh sách rảnh rỗi */
        uint8_t idx = block->order - OS_MIN_ORDER;
        os_block_t **curr = &free_lists[idx];
        while (*curr != NULL && *curr != buddy) {
            curr = &(*curr)->next;
        }
        if (*curr == buddy) {
            *curr = buddy->next;
        }
        
        /* Gộp 2 khối lại: Khối gộp sẽ lấy địa chỉ của khối nằm trước trong RAM */
        if ((uint8_t *)buddy < (uint8_t *)block) {
            block = buddy;
        }
        
        block->order++; /* Kích thước khối tăng gấp đôi */
    }
    
    /* Nạp lại khối RAM (đã gộp to nhất có thể) vào danh sách rảnh rỗi */
    uint8_t final_idx = block->order - OS_MIN_ORDER;
    block->next = free_lists[final_idx];
    free_lists[final_idx] = block;
    
    __asm volatile ("cpsie i");
}