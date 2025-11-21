##
## Makefile para Proyecto Pong STM32F429I-DISC1
## IE0624 - Laboratorio de Microcontroladores
##

# Nombre del proyecto
PROJECT = pong_stm32

# Directorios
SRCDIR = src
INCDIR = include
LIBDIR = lib
BUILDDIR = build
OPENCM3_DIR = libopencm3

# Archivos fuente del proyecto (SIN lcd_graphics.c)
SRCS = $(SRCDIR)/main.c \
       $(SRCDIR)/ultrasonic.c \
       $(SRCDIR)/game_logic.c \
       $(LIBDIR)/clock.c \
       $(LIBDIR)/syscalls.c \
       $(LIBDIR)/sdram.c \
       $(LIBDIR)/lcd-spi.c \
       $(LIBDIR)/console.c

# Archivos objeto
OBJS = $(SRCS:%.c=$(BUILDDIR)/%.o)

# Compilador y herramientas
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
LD = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE = $(PREFIX)size
GDB = $(PREFIX)gdb

# MCU específico
DEVICE = stm32f429i-disco
OPENCM3_TARGET = stm32/f4

# Flags del compilador
ARCH_FLAGS = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS = $(ARCH_FLAGS) \
         -DSTM32F4 \
         -I$(INCDIR) \
         -I$(LIBDIR) \
         -I$(OPENCM3_DIR)/include \
         -Os \
         -g \
         -Wall -Wextra \
         -fno-common \
         -ffunction-sections \
         -fdata-sections \
         -std=c99

# Flags del linker
LDSCRIPT = stm32f429i-disco.ld
LDFLAGS = $(ARCH_FLAGS) \
          -L$(OPENCM3_DIR)/lib \
          -T$(LDSCRIPT) \
          -nostartfiles \
          -Wl,--gc-sections \
          --specs=nano.specs

# Librerías
LDLIBS = -lopencm3_stm32f4 -lm

# Regla por defecto
all: libopencm3 $(BUILDDIR) $(BUILDDIR)/$(PROJECT).elf $(BUILDDIR)/$(PROJECT).bin

# Verificar y compilar libopencm3 si es necesario
libopencm3:
	@if [ ! -f $(OPENCM3_DIR)/lib/libopencm3_stm32f4.a ]; then \
		echo "Compilando LibOpenCM3..."; \
		$(MAKE) -C $(OPENCM3_DIR); \
	fi

# Crear directorios necesarios
$(BUILDDIR):
	mkdir -p $(BUILDDIR)/$(SRCDIR)
	mkdir -p $(BUILDDIR)/$(LIBDIR)

# Compilar archivos objeto del proyecto
$(BUILDDIR)/$(SRCDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar archivos objeto de lib
$(BUILDDIR)/$(LIBDIR)/%.o: $(LIBDIR)/%.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Linker
$(BUILDDIR)/$(PROJECT).elf: $(OBJS)
	@echo "Linking $@"
	$(LD) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	@echo "Size information:"
	$(SIZE) $@

# Crear binario
$(BUILDDIR)/$(PROJECT).bin: $(BUILDDIR)/$(PROJECT).elf
	@echo "Creating binary $@"
	$(OBJCOPY) -O binary $< $@

# Flashear a la placa
flash: $(BUILDDIR)/$(PROJECT).bin
	@echo "Flashing to STM32F429I-DISC1..."
	st-flash write $(BUILDDIR)/$(PROJECT).bin 0x8000000

# Debug con OpenOCD y GDB
debug: $(BUILDDIR)/$(PROJECT).elf
	openocd -f board/stm32f429discovery.cfg &
	$(GDB) $(BUILDDIR)/$(PROJECT).elf -ex "target remote localhost:3333"

# Limpiar archivos generados
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILDDIR)

# Limpiar todo incluyendo libopencm3
distclean: clean
	@echo "Cleaning libopencm3..."
	$(MAKE) -C $(OPENCM3_DIR) clean

# Forzar recompilación de libopencm3
libopencm3-rebuild:
	$(MAKE) -C $(OPENCM3_DIR) clean
	$(MAKE) -C $(OPENCM3_DIR)

# Información del proyecto
info:
	@echo "Project: $(PROJECT)"
	@echo "Source files:"
	@echo "$(SRCS)" | tr ' ' '\n'
	@echo ""
	@echo "Build directory: $(BUILDDIR)"
	@echo "LibOpenCM3: $(OPENCM3_DIR)"

.PHONY: all clean distclean flash debug libopencm3 libopencm3-rebuild info