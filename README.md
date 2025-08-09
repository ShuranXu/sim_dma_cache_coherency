# RISC-V Multi-Core DMA Cache Coherency Demo

This project showcases **non-coherent DMA transfers** on a multi-core RISC-V system, implemented in two flavors:

1. **Linux User-Space Version** — Runs under a RISC-V Linux kernel on QEMU with multiple harts, using POSIX threads.
2. **Bare-Metal Version** — Runs in pure Machine mode on Spike with four harts, using only minimal startup code and MMIO UART output.

Both variants simulate a DMA operation where one hart writes a data buffer, and other harts observe the results after explicit cache-maintenance operations.

---

## Purpose

On architectures without hardware cache coherency between DMA engines and CPU cores, software must:
- **Clean (write-back) caches** before DMA reads from memory.
- **Invalidate caches** after DMA writes to memory.
- Use **synchronization primitives** to ensure ordering between cores.

This demo illustrates those principles in a simple, reproducible setup, highlighting differences in:
- **Privilege level** (user mode under Linux vs. Machine mode bare-metal)
- **Toolchains and environments**
- **Cache-ops availability** (`cbo.*` in M-mode, fences in Linux user-space)
- **I/O mechanism** (stdio vs. UART MMIO)

---

## High-Level Architecture

```
         ┌────────────┐
         │ Hart 0     │
         │------------│
         │ Init src[] │
         │ Clean src  │
         │ "DMA" copy │
         │ Invalidate │
         │ Signal done│
         └─────┬──────┘
               │ atomic flag
               ▼
     ┌─────────────────────┐
     │ Harts 1–3           │
     │---------------------│
     │ Wait for flag       │
     │ Invalidate dst      │
     │ Read & print dst[0] │
     └─────────────────────┘
```

---

## Linux Version (QEMU)

- Builds a minimal RISC-V Linux kernel for the `virt` platform.
- Cross-compiles the `dma_multi_core` C program as a static binary using POSIX threads.
- Packages the binary into an initramfs and boots QEMU with 4 vCPUs.
- Each hart (thread) prints the first 16 bytes of the DMA result.

**Limitations:**
- Runs in user mode → cannot access Machine-mode CSRs or execute `cbo.*` instructions.
- Cache maintenance is simulated via fences and memory ordering.

---

## Bare-Metal Version (Spike)

- No OS — runs in Machine mode directly on the Spike ISA simulator.
- Hart 0 performs initialization, DMA copy, and signals completion.
- Harts 1–3 wait on an atomic flag, then invalidate and read the buffer.
- Output is via direct UART MMIO at `0x10000000`.
- Uses a custom linker script, crt0.S, and minimal libc stubs.

**Advantages over Linux version:**
- Full access to Machine-mode CSRs (e.g., `mhartid`).
- Can exercise `cbo.clean`, `cbo.inval` if compiled with cache-op extensions.

---

## Key Takeaways

- **Cache coherency management** is a critical software task when DMA engines do not automatically keep CPU caches in sync.
- **Privilege level** affects what cache instructions are legal.
- **Atomic operations** are essential for safe cross-hart communication.
- Simulated environments (QEMU, Spike) allow reproducible experiments without hardware.

---

## Repository Structure

```
cache_coherency/
├── linux/                 # Linux user-space demo
│   ├── dma_multi_core.c
│   ├── mkrootfs.sh
│   └── README.md
└── bare_metal/            # Bare-metal M-mode demo
    ├── crt0.S
    ├── dma_multi_core_bare_metal.c
    ├── link.ld
    └── README.md
```

---

## How to Run

See the `README.md` in each subdirectory for:
- Toolchain setup
- Build steps
- Emulator invocation commands
- Example output
