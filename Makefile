CC = m68k-elf-gcc
OBJCOPY = m68k-elf-objcopy
MAME = bin/taito_emu
SPLIT_ROM = bin/split_rom.py
MISTER_HOSTNAME=mister-dev

TARGET = finalb_test
SRCS = init.c main.c interrupts_default.c printf/printf.c


BUILD_DIR = build/$(TARGET)
ORIGINAL_DIR = original_roms

OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:c=o))
BUILD_DIRS = $(sort $(dir $(OBJS))) 
GLOBAL_DEPS = Makefile

DEFINES = -DPRINTF_SUPPORT_DECIMAL_SPECIFIERS=0 \
	-DPRINTF_SUPPORT_EXPONENTIAL_SPECIFIERS=0 \
	-DPRINTF_SUPPORT_LONG_LONG=0 \
	-DPRINTF_ALIAS_STANDARD_FUNCTION_NAMES=1 \
	-DPRINTF_ALIAS_STANDARD_FUNCTION_NAMES_HARD=1

CFLAGS = -march=68000 -ffreestanding $(DEFINES)
LIBS = -lgcc
LDFLAGS = -march=68000 -static -nostdlib

ifeq ($(TARGET),finalb_test)
GAME = finalb
CPU_ROM_LOW = b82-09.10
CPU_ROM_HIGH = b82-17.11
CPU_ROM_SIZE = 0x40000
else ifeq ($(TARGET),finalb)
GAME = finalb
ORIGINAL = 1
else
error
endif

ORIGINAL ?= 0
GAME_DIR = $(BUILD_DIR)/$(GAME)
BUILT_BINS = $(addprefix $(GAME_DIR)/, $(CPU_ROM_LOW) $(CPU_ROM_HIGH))

ifeq ($(ORIGINAL),0)
ROMPATH = ../$(BUILD_DIR);../$(ORIGINAL_DIR)
else
ROMPATH = ../$(ORIGINAL_DIR)
endif

all: $(BUILT_BINS)

$(BUILD_DIR)/cpu.bin: $(BUILD_DIR)/cpu.elf
	$(OBJCOPY) -O binary $< $@

$(GAME_DIR)/$(CPU_ROM_HIGH): $(BUILD_DIR)/cpu.bin | $(GAME_DIR)
	$(SPLIT_ROM) $@ $< 0x00001 $(CPU_ROM_SIZE)

$(GAME_DIR)/$(CPU_ROM_LOW): $(BUILD_DIR)/cpu.bin | $(GAME_DIR)
	$(SPLIT_ROM) $@ $< 0x00000 $(CPU_ROM_SIZE)

$(BUILD_DIR)/%.o: src/%.c $(GLOBAL_DEPS) | $(BUILD_DIRS)
	@echo $@
	@$(CC) -MMD -o $@ $(CFLAGS) -c $<

$(BUILD_DIR)/cpu.elf: $(OBJS)
	@echo $@
	@$(CC) -T linker/$(GAME).ld -o $@ $(LDFLAGS) $^ $(LIBS)

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

flash_low: $(GAME_DIR)/$(MAIN_LOW_BIN)
	minipro -p W27C020 -w $<

flash_high: $(GAME_DIR)/$(MAIN_HIGH_BIN)
	minipro -p W27C020 -w $<

flash_audio_low: $(GAME_DIR)/$(AUDIO_LOW_BIN)
	cat $< $< $< $< > $<.4x
	minipro -p W27C020 -w $<.4x

flash_audio_high: $(GAME_DIR)/$(AUDIO_HIGH_BIN)
	cat $< $< $< $< > $<.4x
	minipro -p W27C020 -w $<.4x

-include $(OBJS:o=d)