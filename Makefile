CC = ia16-elf-gcc
NASM = nasm
OBJCOPY = ia16-elf-objcopy
MAME = bin/m107_flat
SPLIT_ROM = bin/split_rom.py
MISTER_HOSTNAME=mister-dev

TARGET = airass_test
C_SRCS = main.c comms.c interrupts_default.c init.c printf/printf.c
ASM_SRCS = entry.S
NASM_SRCS = tests.asm timing.asm

BUILD_DIR = build/$(TARGET)
ORIGINAL_DIR = original_roms

OBJS = $(addprefix $(BUILD_DIR)/, $(C_SRCS:c=o) $(ASM_SRCS:S=o) $(NASM_SRCS:asm=o))
BUILD_DIRS = $(sort $(dir $(OBJS))) 
GLOBAL_DEPS = Makefile

DEFINES = -DPRINTF_SUPPORT_DECIMAL_SPECIFIERS=0 \
	-DPRINTF_SUPPORT_EXPONENTIAL_SPECIFIERS=0 \
	-DPRINTF_SUPPORT_LONG_LONG=0 \
	-DPRINTF_ALIAS_STANDARD_FUNCTION_NAMES=1 \
	-DPRINTF_ALIAS_STANDARD_FUNCTION_NAMES_HARD=1

CFLAGS = -march=v30 -mcmodel=small -ffreestanding $(DEFINES) -O2
LIBS = -lgcc
LDFLAGS = $(CFLAGS) -static -nostdlib

ifeq ($(TARGET),airass_test)
GAME = airass
CPU_ROM_L0 = f4-a-l0-etc.l0
CPU_ROM_H0 = f4-a-h0-etc.h0
CPU_ROM_SIZE = 0x40000
else ifeq ($(TARGET),airass)
GAME = airass
ORIGINAL = 1
else
error
endif

EPROM_SIZE ?= 0x40000
EPROM_TYPE ?= W27C020

ORIGINAL ?= 0
GAME_DIR = $(BUILD_DIR)/$(GAME)
BUILT_BINS = $(addprefix $(GAME_DIR)/, $(CPU_ROM_L0) $(CPU_ROM_H0))

ifeq ($(ORIGINAL),0)
ROMPATH = ../$(BUILD_DIR);../$(ORIGINAL_DIR)
else
ROMPATH = ../$(ORIGINAL_DIR)
endif

all: $(BUILT_BINS)

$(BUILD_DIR)/cpu.bin: $(BUILD_DIR)/cpu.elf
	$(OBJCOPY) -O binary --change-section-lma .data=0x10000 $< $@

$(GAME_DIR)/$(CPU_ROM_H0): $(BUILD_DIR)/cpu.bin $(SPLIT_ROM) | $(GAME_DIR)
	$(SPLIT_ROM) $@ $< 2 1 $(CPU_ROM_SIZE)

$(GAME_DIR)/$(CPU_ROM_L0): $(BUILD_DIR)/cpu.bin $(SPLIT_ROM) | $(GAME_DIR)
	$(SPLIT_ROM) $@ $< 2 0 $(CPU_ROM_SIZE)

$(BUILD_DIR)/cpu_high_$(EPROM_SIZE).bin: $(BUILD_DIR)/cpu.bin $(SPLIT_ROM) | $(GAME_DIR)
	$(SPLIT_ROM) $@ $< 2 1 $(EPROM_SIZE)

$(BUILD_DIR)/cpu_low_$(EPROM_SIZE).bin: $(BUILD_DIR)/cpu.bin $(SPLIT_ROM) | $(GAME_DIR)
	$(SPLIT_ROM) $@ $< 2 0 $(EPROM_SIZE)


$(BUILD_DIR)/%.o: src/%.c $(GLOBAL_DEPS) | $(BUILD_DIRS)
	@echo $@
	@$(CC) -MMD -o $@ $(CFLAGS) -c $<

$(BUILD_DIR)/%.o: src/%.S $(GLOBAL_DEPS) | $(BUILD_DIRS)
	@echo $@
	@$(CC) -MMD -o $@ $(CFLAGS) -c $<

$(BUILD_DIR)/%.o: src/%.asm $(GLOBAL_DEPS) | $(BUILD_DIRS)
	@echo $@
	@$(NASM) -MD $($@:o=d) -o $@ -f elf -D_TEXT=.text -D_BSS=.bss -D_DATA=.data $<

$(BUILD_DIR)/cpu.elf: $(OBJS) linker/$(GAME).ld
	@echo $@
	@$(CC) -T linker/$(GAME).ld -o $@ $(LDFLAGS) $(OBJS) $(LIBS)
	ia16-elf-objdump -d -M i8086,intel,intel-mnemonic $@ > $(BUILD_DIR)/cpu.asm

$(BUILD_DIRS):
	mkdir -p $@

$(GAME_DIR):
	mkdir -p $@

.PHONY: flash_low flash_high run debug


debug: $(BUILT_BINS)
	mkdir -p mame
	cd mame && ../$(MAME) -window -nomaximize -resolution0 640x480 -debug -rompath "$(ROMPATH)" $(GAME)

trace: $(BUILT_BINS)
	mkdir -p mame
	cd mame && ../$(MAME) -window -nomaximize -resolution0 640x480 -debug -debugscript ../add.scr -rompath "$(ROMPATH)" $(GAME)

run: $(BUILT_BINS)
	mkdir -p mame
	cd mame && ../$(MAME) -window -nomaximize -resolution0 640x480 -rompath "$(ROMPATH)" $(GAME)

flash_low: $(BUILD_DIR)/cpu_low_$(EPROM_SIZE).bin
	minipro -p $(EPROM_TYPE) -w $<

flash_high: $(BUILD_DIR)/cpu_high_$(EPROM_SIZE).bin
	minipro -p $(EPROM_TYPE) -w $<

picorom: $(GAME_DIR)/$(CPU_ROM_H0) $(GAME_DIR)/$(CPU_ROM_L0)
	picorom upload cpu_l0 $(GAME_DIR)/$(CPU_ROM_L0) 2mbit
	picorom upload cpu_h0 $(GAME_DIR)/$(CPU_ROM_H0) 2mbit

-include $(OBJS:o=d)