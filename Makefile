CC = ia16-elf-gcc
OBJCOPY = ia16-elf-objcopy
MAME = bin/irem_emu
SPLIT_ROM = bin/split_rom.py
NASM = nasm
MISTER_HOSTNAME=mister-dev

TARGET = gunforce_test
C_SRCS = main.c comms.c interrupts_default.c init.c printf/printf.c
ASM_SRCS = entry.S
NASM_SRCS = timing.asm

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

ifeq ($(TARGET),gunforce_test)
GAME = gunforcetb
CPU_ROM_L0 = gf_l0-d.tb
CPU_ROM_H0 = gf_h0-d.tb
AUDIO_ROM_L0 = gf_sl0.rom
AUDIO_ROM_H0 = gf_sh0.rom
CPU_ROM_SIZE = 0x40000
AUDIO_ROM_SIZE = 0x10000
else ifeq ($(TARGET),gunforce)
GAME = gunforceu
ORIGINAL = 1
else ifeq ($(TARGET),rtypeleo)
GAME = rtypeleo
ORIGINAL = 1
else
error
endif

EPROM_SIZE ?= 0x40000
EPROM_TYPE ?= W27C020

ORIGINAL ?= 0
GAME_DIR = $(BUILD_DIR)/$(GAME)
BUILT_BINS = $(addprefix $(GAME_DIR)/, $(CPU_ROM_L0) $(CPU_ROM_H0) $(AUDIO_ROM_L0) $(AUDIO_ROM_H0))

ifeq ($(ORIGINAL),0)
ROMPATH = ../$(BUILD_DIR);../$(ORIGINAL_DIR)
else
ROMPATH = ../$(ORIGINAL_DIR)
endif

all: $(BUILT_BINS)

$(BUILD_DIR)/cpu.bin: $(BUILD_DIR)/cpu.elf
	$(OBJCOPY) -O binary --change-section-lma .data=0x10000 $< $@

$(BUILD_DIR)/audio.bin: src/audio.asm | $(BUILD_DIR)
	$(NASM) -f bin -o $@ -MD ${BUILD_DIR}/audio.d -l $(BUILD_DIR)/audio.lst $<

$(GAME_DIR)/$(CPU_ROM_H0): $(BUILD_DIR)/cpu.bin $(SPLIT_ROM) | $(GAME_DIR)
	$(SPLIT_ROM) $@ $< 2 1 $(CPU_ROM_SIZE)

$(GAME_DIR)/$(CPU_ROM_L0): $(BUILD_DIR)/cpu.bin $(SPLIT_ROM) | $(GAME_DIR)
	$(SPLIT_ROM) $@ $< 2 0 $(CPU_ROM_SIZE)

$(GAME_DIR)/$(AUDIO_ROM_H0): $(BUILD_DIR)/audio.bin $(SPLIT_ROM) | $(GAME_DIR)
	$(SPLIT_ROM) $@ $< 2 1 $(AUDIO_ROM_SIZE)

$(GAME_DIR)/$(AUDIO_ROM_L0): $(BUILD_DIR)/audio.bin $(SPLIT_ROM) | $(GAME_DIR)
	$(SPLIT_ROM) $@ $< 2 0 $(AUDIO_ROM_SIZE)

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

$(BUILD_DIRS):
	mkdir -p $@

$(GAME_DIR):
	mkdir -p $@

.PHONY: flash_low flash_high run debug $(GAME_DIR) 


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

picorom: $(BUILT_BINS)
	picorom upload cpu_l0 $(GAME_DIR)/$(CPU_ROM_L0) 2mbit
	picorom upload cpu_h0 $(GAME_DIR)/$(CPU_ROM_H0) 2mbit
	picorom upload audio_l0 $(GAME_DIR)/$(AUDIO_ROM_L0) 512kbit
	picorom upload audio_h0 $(GAME_DIR)/$(AUDIO_ROM_H0) 512kbit

-include $(OBJS:o=d)
-include $(BUILD_DIR)/audio.d

