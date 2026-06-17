#ifndef STM32F100_H
#define STM32F100_H

#include <stdint.h>

/* =========================================================================
 * 1. ĐỊNH NGHĨA CÁC ĐỊA CHỈ GỐC (BASE ADDRESSES)
 * ========================================================================= */
#define PERIPH_BASE     0x40000000U
#define APB2PERIPH_BASE (PERIPH_BASE + 0x10000U)
#define AHBPERIPH_BASE  (PERIPH_BASE + 0x18000U)

#define RCC_BASE        (AHBPERIPH_BASE + 0x9000U)  /* Khối cấp xung nhịp */
#define GPIOA_BASE      (APB2PERIPH_BASE + 0x0800U) /* Khối chân cắm Port A */
#define USART1_BASE     (APB2PERIPH_BASE + 0x3800U) /* Khối truyền thông UART1 */

/* =========================================================================
 * 2. CẤU TRÚC THANH GHI CẤP XUNG NHỊP (RCC)
 * ========================================================================= */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t APB1RSTR;
    volatile uint32_t AHBENR;
    volatile uint32_t APB2ENR;      /* Kích hoạt xung nhịp cho APB2 (GPIOA, USART1) */
    volatile uint32_t APB1ENR;
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
} RCC_TypeDef;

#define RCC ((RCC_TypeDef *) RCC_BASE)

/* =========================================================================
 * 3. CẤU TRÚC THANH GHI CHÂN CẮM (GPIO)
 * ========================================================================= */
typedef struct {
    volatile uint32_t CRL;      /* Cấu hình chân 0-7 */
    volatile uint32_t CRH;      /* Cấu hình chân 8-15 (Chân PA9 và PA10 của UART1 nằm ở đây) */
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *) GPIOA_BASE)

/* =========================================================================
 * 4. CẤU TRÚC THANH GHI UART (USART1)
 * ========================================================================= */
typedef struct {
    volatile uint32_t SR;       /* Thanh ghi trạng thái (Báo cờ truyền/nhận xong) */
    volatile uint32_t DR;       /* Thanh ghi dữ liệu (Chứa ký tự cần gửi/nhận) */
    volatile uint32_t BRR;      /* Thanh ghi tốc độ Baud */
    volatile uint32_t CR1;      /* Thanh ghi điều khiển 1 */
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

#define USART1 ((USART_TypeDef *) USART1_BASE)

/* =========================================================================
 * 5. CẤU TRÚC THANH GHI LÕI CORTEX-M3 (SYSTICK)
 * ========================================================================= */
#define SCS_BASE        (0xE000E000U)
#define SYSTICK_BASE    (SCS_BASE + 0x0010U)

typedef struct {
    volatile uint32_t CTRL;     /* Thanh ghi điều khiển và trạng thái */
    volatile uint32_t LOAD;     /* Thanh ghi giá trị nạp lại (Reload Value) */
    volatile uint32_t VAL;      /* Thanh ghi giá trị hiện tại (Current Value) */
    volatile uint32_t CALIB;    /* Thanh ghi hiệu chuẩn */
} SysTick_TypeDef;

#define SysTick ((SysTick_TypeDef *) SYSTICK_BASE)
#endif /* STM32F100_H */