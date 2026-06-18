# =========================================================================
# MAKEFILE BIÊN DỊCH DỰ ÁN RTOS STM32F100 (CORTEX-M3)
# =========================================================================

TARGET = os_kernel

# Bộ công cụ GNU Arm
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size

BUILD_DIR = build

# Liệt kê danh sách file C và Assembly
C_SOURCES = \
src/syscalls.c \
src/stm32f100_uart.c \
src/os_kernel.c \
src/main.c

ASM_SOURCES = \
startup/startup_stm32f100.s

# Kiến trúc Cortex-M3
MCU = -mcpu=cortex-m3 -mthumb
C_INCLUDES = -Iinclude

# Tham số biên dịch (-O0 để không tối ưu, dễ debug)
CFLAGS = $(MCU) $(C_INCLUDES) -O0 -Wall -fdata-sections -ffunction-sections -g -gdwarf-2
ASFLAGS = $(MCU) $(CFLAGS)

# Kịch bản liên kết bộ nhớ
LDSCRIPT = ld/STM32F100_FLASH.ld
LIBS = -lc -lm -lnosys
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--gc-sections

# =========================================================================
# QUY TẮC BUILD
# =========================================================================
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

# Gom file đối tượng (.o)
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "Compiling: $<"
	@$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	@echo "Assembling: $<"
	@$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	@echo "Linking: $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@$(SZ) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	@$(CP) -O binary -S $< $@
	@echo "========================================================="
	@echo "BIÊN DỊCH THÀNH CÔNG! File đầu ra đã sẵn sàng trong /build"
	@echo "========================================================="

$(BUILD_DIR):
	mkdir -p $@ 

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean