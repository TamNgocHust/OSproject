/* Chỉ định điểm bắt đầu của chương trình là hàm Reset_Handler trong file startup */
ENTRY(Reset_Handler)

/* Khai báo kích thước đỉnh Stack (Căn chỉnh 8-byte theo chuẩn ARM EABI) */
_estack = 0x20002000;    /* Đỉnh RAM (0x20000000 + 8KB) */

/* Khai báo các phân vùng bộ nhớ vật lý của vi điều khiển STM32F100 */
MEMORY
{
  FLASH (rx)      : ORIGIN = 0x00000000, LENGTH = 128K
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 8K
}

/* Định nghĩa cấu trúc các vùng dữ liệu */
SECTIONS
{
  /* 1. Vùng chứa Bảng vector ngắt (isr_vector) và Mã lệnh chương trình (.text) */
  .text :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector)) /* Ép giữ lại bảng vector ngắt, không cho compiler xóa */
    *(.text)           /* Chứa toàn bộ mã lệnh C/Assembly */
    *(.text*)
    
    /* Các đoạn mã chuyển đổi kiến trúc */
    *(.glue_7)         
    *(.glue_7t)        

    . = ALIGN(4);
    _etext = .;        /* Đánh dấu điểm kết thúc của vùng mã lệnh */
  } > FLASH

  /* 2. Vùng chứa Hằng số và Chuỗi ký tự (Read-only data) */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* Chứa chuỗi như "Hello World!\n" */
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  /* Đánh dấu địa chỉ Load Address (LMA) của vùng .data trên Flash */
  _sidata = LOADADDR(.data);

  /* 3. Vùng chứa Biến toàn cục có khởi tạo (.data) */
  /* Vùng này nằm trên RAM khi chạy (VMA), nhưng được nạp từ Flash (LMA) */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* Đánh dấu điểm bắt đầu vùng .data trên RAM */
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;        /* Đánh dấu điểm kết thúc vùng .data trên RAM */
  } > RAM AT > FLASH

  /* 4. Vùng chứa Biến toàn cục không khởi tạo (.bss) mặc định bằng 0 */
  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    _sbss = .;         /* Đánh dấu điểm bắt đầu vùng .bss */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;         /* Đánh dấu điểm kết thúc vùng .bss */
    __bss_end__ = _ebss;
  } > RAM

  /* 5. Cấu hình kiểm tra an toàn tràn bộ nhớ RAM */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + 0x400;     /* Giả lập một khoảng trống tối thiểu cho Heap/Stack (1KB) */
    . = ALIGN(8);
  } > RAM

  /* Xóa các phân vùng rác không cần thiết của compiler */
  /DISCARD/ :
  {
    *(.note*)
    *(.comment*)
  }
}

/* Lệnh kiểm soát an toàn: Dừng biên dịch ngay nếu RAM bị đầy */
ASSERT((_ebss + 0x400) <= _estack, "RAM overflow: .data/.bss exceed STM32F100 SRAM")