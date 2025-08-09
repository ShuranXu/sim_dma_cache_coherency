# Linux-based RISC-V DMA Multi-Core Demo

This project demonstrates a non-coherent DMA transfer on a multi-core RISC-V system using POSIX threads under a Linux kernel emulator.

## Overview

* We build a standard RISC-V Linux kernel targeting the `virt` machine.
* We compile `dma_multi_core.c` as a statically-linked Linux user-space binary.
* We package the binary into a minimal initramfs so it runs as PID 1 under QEMU.
* We boot the kernel + initramfs under QEMU with 4 vCPUs and observe each hart reading its DMA results.

**Limitation:**

* The emulated environment uses the Base (I), Multiply/Divide (M), Atomic (A), Floating-Point (F/D), and Compressed (C) extensions only (`rv64g`), not the Machine-mode-specific CSRs or Zicbo CMO cache instructions: those trap as illegal in user mode.

## Prerequisites

* **RISC-V Linux cross-toolchain** installed (e.g., `riscv64-unknown-linux-gnu-gcc`).
* **QEMU system emulator** with RISC-V support (`qemu-system-riscv64`).
* `git`, `make`, `cpio`, and root privileges (for `mknod`) on the host.

## Directory Layout

```
cache_coherency/
├── dma_multi_core.c       # DMA demo source code
├── mkrootfs.sh            # Script to build initramfs
├── initramfs/             # Generated initramfs tree
└── rootfs.cpio            # Packed initramfs image
```

## 1. Build the Linux Kernel

```bash
export WORK=$HOME/rv-work
export CROSS=riscv64-unknown-linux-gnu
export PATH=$HOME/riscv/bin:$PATH

# Clone kernel
cd $WORK
git clone --depth 1 https://git.kernel.org/.../linux-stable.git linux
cd linux

# Configure & build
make ARCH=riscv defconfig
make ARCH=riscv \
     CROSS_COMPILE=$CROSS- \
     -j$(nproc) Image
```

* Output kernel image: `arch/riscv/boot/Image`

## 2. Compile the DMA Demo

```bash
cd $WORK/cache_coherency

$CROSS-gcc -static -std=gnu99 -O2 \
  -march=rv64g -mabi=lp64 \
  dma_multi_core.c -o dma_multi_core \
  -pthread
```

* `-march=rv64g` ensures compatibility with QEMU’s virt CPU (`acdfim,c`).

## 3. Build the Initramfs

```bash
chmod +x mkrootfs.sh
sudo ./mkrootfs.sh
```

**mkrootfs.sh** performs:

1. Cleans old `initramfs/` and `rootfs.cpio`.
2. Creates `initramfs/{bin,dev,proc,sys}`.
3. Copies `dma_multi_core` to `initramfs/bin`.
4. Creates `/dev/console` device node.
5. Writes `/init` shell wrapper that mounts `/proc` and `/sys` and execs the demo.
6. Packs `initramfs` into `rootfs.cpio`.

## 4. Boot Under QEMU

```bash
qemu-system-riscv64 \
  -machine virt \
  -nographic \
  -kernel $WORK/linux/arch/riscv/boot/Image \
  -initrd rootfs.cpio \
  -append "console=ttyS0 init=/init" \
  -smp 4 \
  -m 2G
```

* `-machine virt` uses the generic RISC-V virtual platform.
* `-nographic` routes the serial console to the terminal.
* `-smp 4` boots four vCPUs (harts).
* `init=/init` tells Linux to run the demo as PID 1.

## Expected Output

```
Allocated src and dst buffers
Initialized src buffer
Cleaned cache lines for src buffer
Launched DMA simulator
Waiting for threads to finish
[hart 1] dst_buf[0..15]: 00 01 02 ...
[hart 2] dst_buf[0..15]: 00 01 02 ...
[hart 3] dst_buf[0..15]: 00 01 02 ...
[hart 0] dst_buf[0..15]: 00 01 02 ...
All threads done; spinning forever so init() never exits.
```

## Limitations

* **No Machine-mode CSRs in user space:**
  Instructions like `csrr mhartid` trap as illegal in user mode under Linux.
* **No Zicbo cache-ops on QEMU virt:**
  The `cbo.clean` and `cbo.inval` instructions are unsupported and must be replaced with fences.

---

For any issues, verify that:

* Your kernel and demo are compiled for the same ISA (`rv64g`).
* `rootfs.cpio` contains `/init` and `/bin/dma_multi_core`.
* You’re passing `init=/init` in the QEMU `-append` line.
