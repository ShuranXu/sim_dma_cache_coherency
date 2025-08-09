# Makefile for dma_multi_core example

# Customize these variables as needed
RISCV        ?= $(HOME)/riscv
RISCV_PREFIX ?= riscv64-unknown-linux-gnu
CC           := $(RISCV)/bin/$(RISCV_PREFIX)-gcc
SPIKE        := $(RISCV)/bin/spike
PK 			 := $(RISCV)/riscv-pk/build/pk
BBL          := $(RISCV)/riscv-pk/build/bbl
RAMDISK      := rootfs.cpio
ISA          := rv64imac_zicsr_zicbom_zicbop_zicboz
CFLAGS       := -std=c99 -O2 \
                 -march=rv64gc \
                 -mabi=lp64
LDFLAGS      := -static -pthread
SRC          := dma_multi_core.c
TARGET       := dma_multi_core

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

run: $(TARGET)
	@echo "Launching on Spike (4 harts)..."
	$(SPIKE) \
		--isa=$(ISA) \
    	$(BBL) $(RAMDISK)

clean:
	rm -f $(TARGET)
