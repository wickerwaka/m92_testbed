CC = ia16-elf-gcc
OBJCOPY = ia16-elf-objcopy
MAME = bin/irem_emu
SPLIT_ROM = bin/split_rom.py
MISTER_HOSTNAME=mister-dev

TARGET = bomberman_test
C_SRCS = main.c comms.c interrupts_default.c init.c printf/printf.c
ASM_SRCS = entry.S

BUILD_DIR = build/$(TARGET)
ORIGINAL_DIR = original_roms

OBJS = $(addprefix $(BUILD_DIR)/, $(C_SRCS:c=o) $(ASM_SRCS:S=o))
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

ifeq ($(TARGET),bomberman)
GAME = bombrman
ORIGINAL = 1
else ifeq ($(TARGET),bomberman_test)
GAME = bombrman
CPU_ROM_L0 = bbm-p0.ic65
CPU_ROM_H0 = bbm-p1.ic62
CPU_ROM_SIZE = 0x20000
CFLAGS += -DM90=1
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

ifeq ($(ORIGINAL),0)
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

$(BUILD_DIR)/cpu.elf: $(OBJS) linker/$(GAME).ld
	@echo $@
	@$(CC) -T linker/$(GAME).ld -o $@ $(LDFLAGS) $(OBJS) $(LIBS)
endif


$(BUILD_DIRS):
	mkdir -p $@

$(GAME_DIR):
	mkdir -p $@

.PHONY: flash_low flash_high run debug


debug: $(BUILT_BINS)
	mkdir -p mame
	cd mame && ../$(MAME) -window -nomaximize -resolution0 640x480 -debug -rompath "$(ROMPATH)" $(GAME)

run: $(BUILT_BINS)
	mkdir -p mame
	cd mame && ../$(MAME) -window -nomaximize -resolution0 640x480 -rompath "$(ROMPATH)" $(GAME)

flash_low: $(BUILD_DIR)/cpu_low_$(EPROM_SIZE).bin
	minipro -p $(EPROM_TYPE) -w $<

flash_high: $(BUILD_DIR)/cpu_high_$(EPROM_SIZE).bin
	minipro -p $(EPROM_TYPE) -w $<

picorom: $(GAME_DIR)/$(CPU_ROM_H0) $(GAME_DIR)/$(CPU_ROM_L0)
	picorom upload cpu_low $(GAME_DIR)/$(CPU_ROM_L0) 1mbit
	picorom upload cpu_high $(GAME_DIR)/$(CPU_ROM_H0) 1mbit

-include $(OBJS:o=d)