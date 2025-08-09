
# Bare-metal M-mode Makefile for dma_multi_core example

# Toolchain & Spike
RISCV        ?= $(HOME)/riscv
CROSS        ?= riscv64-unknown-elf
CC           := $(RISCV)/bin/$(CROSS)-gcc
PK           := $(RISCV)/riscv-pk/build/pk
SPIKE        := $(RISCV)/bin/spike

# RISC-V ISA (must include zicsr + CMO if your code uses them)
ISA          := rv64imac_zicsr_zicbom_zicbop_zicboz

# Linker script and startup
LINKSCRIPT   := link.ld
CRT0         := crt0.S

# Source files
SRC          := dma_multi_core.c
TARGET       := dma_multi_core.elf

# Compiler flags
# -nostdlib: 	tells the toolchain not to automatically link in the standard C runtime (CRT) or the C standard library.
# We are providing our own crt0.S startup stub and write syscall, so no need for usual glibc/newlib startup code.
# -march/-mabi: specifies the exact RISC-V instruction set extensions to target. We are enableing 64-bit, integer,
# multiplication/divide, atomic, compressed, CSR, and the Zicbo cache-maintenance ops.
# -mabi=lp64: l – long” and “p – pointers” are 64 bits wide.
# -O2: optimize
# -mcmodel=medany for large code model (M-mode, high addresses): selects the “medium/any” code model, allowing
# the program text to be placed anywhere in the 64-bit address space (even above 2 GiB).
CFLAGS       := -nostdlib \
                -march=$(ISA) \
                -mabi=lp64 \
                -mcmodel=medany \
                -O2

# Linker flags: use custom linker script
LDFLAGS      := -T $(LINKSCRIPT)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(CRT0) $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# -p4 tells Spike to start 4 harts in bare-metal (machine-mode) execution
run: $(TARGET)
	@echo "Launching on Spike (Machine-mode bare-metal)..."
	$(SPIKE) \
	  --isa=$(ISA) \
	  -p4 \
	  ./$(TARGET)

clean:
	rm -f $(TARGET)
