#!/usr/bin/env bash
set -euo pipefail

# 1) Clean any previous initramfs
rm -rf initramfs rootfs.cpio

# 2) Create the minimal directory structure
mkdir -p initramfs/{bin,dev,proc,sys}

# 3) Install your demo as /init
install -m 0755 dma_multi_core initramfs/init

# 4) Also expose it under /bin for consistency
ln -sf /init initramfs/bin/dma_multi_core

# 5) Create the console device node
#    (requires root; you can also omit if your qemu auto-creates /dev/console)
sudo mknod initramfs/dev/console c 5 1
sudo chmod 600 initramfs/dev/console

# 6) Packages up the initramfs/ directory into a single rootfs.cpio archive that:
#  - Contains the full directory tree
#  - Uses the newc (ASCII) cpio format that Linux understands for initramfs
#  - Marks every file as root-owned so that the embedded /init, /bin/dma_multi_core,
#    and /dev/console all have the correct ownership when the kernel unpacks them.
(
  cd initramfs
  find . | cpio -o -H newc --owner root:root
) > rootfs.cpio

echo "Wrote rootfs.cpio ($(du -h rootfs.cpio | cut -f1))"

# export RISCV=$HOME/riscv
# export CROSS=riscv64-unknown-linux-gnu
# export PATH=$RISCV/bin:$PATH

