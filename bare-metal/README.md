# Bare-metal RISC-V DMA Multi-Core Demo

This project demonstrates a non-coherent DMA transfer on a bare-metal, multi-core RISC-V system running in **Machine mode** under Spike.

## Overview

This project demonstrates how to run a simple multi‐hart “DMA” simulation entirely in RISC-V Machine-mode, with no operating system. Four hardware threads (harts) boot under Spike:

- Hart 0
    * Initializes a 1 KiB source buffer (src_buf[i] = i).
    * Performs a software “DMA” copy into dst_buf (via memcpy), surrounding it with cache-maintenance fences.
    * Signals completion by atomically storing dma_done = 1 with release semantics.

- Harts 1–3
    * Spin, waiting on an atomic acquire load of dma_done.
    * Once they see the flag flip, they invalidate their local caches and print the first 16 bytes of dst_buf.

All console output is driven via direct MMIO writes to the virt-platform UART at `0x1000_0000`, and each hart’s ID (read from the `mhartid` CSR) is prepended to its messages. At the end, each hart enters a low-power `wfi` loop. </br>This minimal setup (startup in crt0.S, custom linker script, and a tiny write_stub.c) shows how to build and synchronize lock-free code on bare-metal RISC-V hardware.

---

## Prerequisites

- **RISC-V bare-metal toolchain**  
  Install `riscv64-unknown-elf-gcc` (newlib) and friends, e.g. via the [riscv-gnu-toolchain](https://github.com/riscv/riscv-gnu-toolchain).  
  Set `RISCV` to its install prefix in your shell:
  ```bash
  export RISCV=$HOME/riscv
  export PATH=$RISCV/bin:$PATH
  git clone https://github.com/riscv/riscv-gnu-toolchain.git "$RISCV/riscv-gnu-toolchain"
  pushd "$RISCV/riscv-gnu-toolchain"
  ./configure --prefix="$RISCV" --enable-multilib
  make linux -j"$(nproc)"
  make install
  popd
  ```
- **Spike**  
  Build from source (`riscv-isa-sim`) with:
  ```bash
  git clone https://github.com/riscv-software-src/riscv-isa-sim.git
  cd riscv-isa-sim
  mkdir build && cd build
  ../configure --prefix=$RISCV
  make -j$(nproc) install
  ```

---

## File Structure

```
.
├── Makefile                     # build & run in M-mode under Spike
├── link.ld                      # custom linker script (ENTRY=_start, .tohost/.fromhost, stack)
├── crt0.S                       # minimal M-mode startup: set up SP, call main
├── dma_multi_core_bare_metal.c  # per-hart branches, buffer init, atomic flag, output
└── write_stub.c                 # UART MMIO & tohost symbols, minimal console_puts()
```

---

## Build & Run

From this directory:

```bash
make clean all
make run
```

This invokes:

1. **Compile & link**  
   ```bash
   riscv64-unknown-elf-gcc -nostdlib  \
   -march=rv64imac_zicsr_zicbom_zicbop_zicboz  \
   -mabi=lp64 -mcmodel=medany \
   -O2 \
   -T link.ld  \
   crt0.S dma_multi_core.c \
   -o dma_multi_core.elf
   ```
2. **Run under Spike** in pure M-mode, 4 harts:
   ```bash
   spike \
   --isa=rv64imac_zicsr_zicbom_zicbop_zicboz \
   -p4 \
   ./dma_multi_core.elf
   ```

---

## Program Flow

1. **Startup**  
   - Spike boots each hart in M-mode at the ELF entrypoint (`_start` in `crt0.S`).  
   - `crt0.S` sets up the stack pointer, then calls `main()`.

2. **Hart ID**  
   - `main()` executes `read_hartid()` (reads the `mhartid` CSR) to get 0–3.

3. **Console message**  
   - Each hart prints `[hart N] starting` via `console_puts()` → UART MMIO.

4. **Hart 0 branch**  
   - Initializes `src_buf[i] = i` for `i = 0…1023`.  
   - Calls `cache_clean()` (fence) on `src_buf`.  
   - Busy-wait loop to simulate DMA latency.  
   - `memcpy(src_buf→dst_buf)` and `cache_invalidate()` on `dst_buf`.  
   - Atomically store `dma_done = 1` with **release** semantics.  
   - Print `[hart 0] DMA done`.

5. **Harts 1–3 branch**  
   - Print `[hart N] wait for DMA`.  
   - Spin on `__atomic_load_n(&dma_done, __ATOMIC_ACQUIRE)`.  
   - After release/acquire synchronizes, call `cache_invalidate(dst_buf)`.  
   - Print `[hart N] dst_buf[0..15]:` then the bytes `00`–`0F` in hex.

6. **Shutdown**  
   - All harts store `tohost = 1` (Spike semi-hosting symbol) to signal exit.  
   - Enter an infinite `wfi` loop; Spike sees `tohost!=0` and terminates.

---

## Expected Output

```
Launching on Spike (Machine-mode bare-metal)...
/home/shuran/riscv/bin/spike   --isa=rv64imac_zicsr_zicbom_zicbop_zicboz   -p4   ./dma_multi_core.elf
[hart 0] starting
[hart 0] initialize source and start DMA
[hart 1] starting
[hart 1] wait for DMA 
[hart 2] starting
[hart 2] wait for DMA 
[hart 3] starting
[hart 3] wait for DMA 
[hart 0] DMA done
[hart 1] dst_buf[0..15]:
 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
[hart 2] dst_buf[0..15]:
 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
[hart 3] dst_buf[0..15]:
 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
```
