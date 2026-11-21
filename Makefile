# Makefile — esp32c3-blink + FreeRTOS (bare metal, no ESP-IDF build system)
TC      := /Users/theo-richard/workspace/toolchains/riscv32-esp-elf/bin/riscv32-esp-elf-
CC      := $(TC)gcc
OBJCOPY := $(TC)objcopy
OBJDUMP := $(TC)objdump
NM      := $(TC)nm
SIZE    := $(TC)size

TARGET  := esp32c3-blink
BUILD   := build

CPU     := -march=rv32imc_zicsr_zifencei -mabi=ilp32 -mcmodel=medany
CFLAGS  := $(CPU) -O2 -g -Wall -ffunction-sections -fdata-sections -fno-common \
           -ffreestanding -fno-builtin -fno-stack-protector -msmall-data-limit=0 \
           -IDrivers/FreeRTOS/baremetal \
           -IDrivers/FreeRTOS/baremetal/include \
           -IDrivers/FreeRTOS/baremetal/port \
           -IDrivers/FreeRTOS/FreeRTOS-Kernel/include \
           -IDrivers/FreeRTOS/FreeRTOS-Kernel/include/freertos \
           -IDrivers/FreeRTOS/esp_additions \
           -IDrivers/FreeRTOS/esp_additions/include \
           -IDrivers/idf/soc_c3/include \
           -IDrivers/idf/soc_c3/register \
           -IDrivers/idf/soc_shared/include \
           -IDrivers/idf/hal_c3/include \
           -IDrivers/idf/hal_shared/include \
           -IDrivers/idf/esp_common/include \
           -IDrivers/idf/esp_rom_c3/include \
           -IDrivers/idf/esp_rom_shared/include \
           -IDrivers/idf/esp_hal_uart-c3 \
           -IDrivers/idf/esp_hal_gpio-c3 \
           -IDrivers/idf
LDFLAGS := $(CPU) -nostdlib -nostartfiles -Wl,--gc-sections -Wl,--no-relax \
           -Wl,-T,ld/esp32c3.ld -Wl,--build-id=none -Wl,-Map=$(BUILD)/$(TARGET).map

RTOS    := Drivers/FreeRTOS
KERNEL  := $(RTOS)/FreeRTOS-Kernel

SRCS    := main/main.c \
           startup/startup_c3.S \
           $(RTOS)/baremetal/heap_4.c \
           $(RTOS)/baremetal/libc_stubs.c \
           $(RTOS)/baremetal/port/port.c \
           $(RTOS)/baremetal/port/portasm.S \
           $(KERNEL)/tasks.c \
           $(KERNEL)/queue.c \
           $(KERNEL)/list.c \
           $(KERNEL)/timers.c \
           $(KERNEL)/event_groups.c \
           $(KERNEL)/stream_buffer.c

OBJS    := $(patsubst %.c,$(BUILD)/%.o,$(patsubst %.S,$(BUILD)/%.o,$(SRCS)))

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin $(BUILD)/$(TARGET).esp.bin verify

$(BUILD)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/$(TARGET).esp.bin: $(BUILD)/$(TARGET).elf
	ENTRY=$$($(NM) $< | awk '/ T _start$$/ {print $$1}'); \
	python3 tools/mkimage.py --elf $< --entry 0x$$ENTRY --chip c3 --out $@

verify: $(BUILD)/$(TARGET).esp.bin
	python3 tools/verify.py --elf $(BUILD)/$(TARGET).elf \
		--bin $(BUILD)/$(TARGET).esp.bin --nm $(NM) --objdump $(OBJDUMP)

dis: $(BUILD)/$(TARGET).elf
	$(OBJDUMP) -d $< > $(BUILD)/$(TARGET).dis

symbols: $(BUILD)/$(TARGET).elf
	$(NM) $< > $(BUILD)/$(TARGET).sym

size: $(BUILD)/$(TARGET).elf
	$(SIZE) $<

clean:
	rm -rf $(BUILD)

.PHONY: all verify dis symbols size clean
