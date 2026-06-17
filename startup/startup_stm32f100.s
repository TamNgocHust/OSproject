.syntax unified
.cpu cortex-m3
.thumb

/* Khai báo các địa chỉ từ file Linker Script (Bước 3) */
.word _sdata     /* Đầu vùng .data trên RAM */
.word _edata     /* Cuối vùng .data trên RAM */
.word _sidata    /* Đầu vùng .data trên Flash (Load Address) */
.word _sbss      /* Đầu vùng .bss trên RAM */
.word _ebss      /* Cuối vùng .bss trên RAM */
.word _estack    /* Đỉnh ngăn xếp (Cuối RAM) */

.global Default_Handler
.global Reset_Handler

/* =========================================================
 * NHIỆM VỤ 1 & 2 & 3: BẢNG VECTOR NGẮT (Nằm ở đầu Flash)
 * ========================================================= */
.section .isr_vector, "a", %progbits
.type g_pfnVectors, %object
g_pfnVectors:
    .word _estack           /* Địa chỉ thứ 1 (0x08000000): Nạp con trỏ ngăn xếp (MSP) */
    .word Reset_Handler     /* Địa chỉ thứ 2 (0x08000004): Nạp địa chỉ Reset_Handler */
    
    /* Các vector ngắt hệ thống khác (Có thể bổ sung sau) */
    .word Default_Handler   /* NMI */
    .word Default_Handler   /* Hard Fault */
    .word Default_Handler   /* Memory Management Fault */
    .word Default_Handler   /* Bus Fault */
    .word Default_Handler   /* Usage Fault */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word 0                 /* Reserved */
    .word Default_Handler   /* SVCall (Quan trọng cho RTOS) */
    .word Default_Handler   /* Debug Monitor */
    .word 0                 /* Reserved */
    .word PendSV_Handler   /* PendSV (Chuyển ngữ cảnh RTOS) */
    .word SysTick_Handler   /* SysTick (Nhịp thời gian RTOS) */
.size g_pfnVectors, . - g_pfnVectors

/* =========================================================
 * NHIỆM VỤ 4: HÀM RESET_HANDLER (Khởi tạo RAM và gọi main)
 * ========================================================= */
.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    /* 4.1 Copy dữ liệu vùng .data từ Flash sang RAM */
    ldr r0, =_sdata      /* Địa chỉ đích (RAM) */
    ldr r1, =_edata      /* Kích thước / Giới hạn */
    ldr r2, =_sidata     /* Địa chỉ nguồn (Flash) */
    movs r3, #0
    b LoopCopyDataInit

CopyDataInit:
    ldr r4, [r2, r3]     /* Đọc từ Flash */
    str r4, [r0, r3]     /* Ghi ra RAM */
    adds r3, r3, #4      /* Nhảy 4 bytes */

LoopCopyDataInit:
    adds r4, r0, r3
    cmp r4, r1
    bcc CopyDataInit

    /* 4.2 Xóa vùng .bss về 0 */
    ldr r2, =_sbss
    ldr r4, =_ebss
    movs r3, #0
    b LoopFillZerobss

FillZerobss:
    str  r3, [r2]        /* Ghi số 0 vào RAM */
    adds r2, r2, #4

LoopFillZerobss:
    cmp r2, r4
    bcc FillZerobss

    /* 4.3 Nhảy vào hàm main() của hệ điều hành */
    bl main

    /* Mắc kẹt an toàn ở đây nếu main() vô tình return */
LoopForever:
    b LoopForever

/* Hàm ngắt ảo để tránh lỗi nếu ngắt xảy ra mà chưa code hàm xử lý */
.section .text.Default_Handler,"ax",%progbits
.type Default_Handler, %function
Default_Handler:
    b Default_Handler