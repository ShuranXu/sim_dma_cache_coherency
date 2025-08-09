# QEMU’s “virt” CPU only implements the base and floating-point extensions (acdfim)
# — it does not support Zicsr or any of the Zicbo CMO cache-ops. So:
# Every cbo.clean/cbo.inval and your csrr ...,mhartid is an illegal opcode on QEMU → SIGILL.

#   -kernel ~/linux/arch/riscv/boot/Image : Load that Linux kernel image at the guest’s reset vector (0x8020_0000 on “virt” by default).
#   -initrd rootfs.cpio : Attach the rootfs.cpio as an initial ramdisk; the kernel will unpack and mount it early in boot.
#   -append "console=ttyS0 init=/init" : pass these kernel command-line arguments:
#     - console=ttyS0 ⇒ send kernel messages and console I/O to the first serial port.
#     - init=/init ⇒ run /init (from the ramdisk) as PID 1.
#   -smp 4 : Present a quad-core CPU to the guest (4 hardware threads/harts).
#   -m 2G : Allocate 2 GiB of RAM for the guest’s physical memory.
qemu-system-riscv64 \
  -machine virt \
  -nographic \
  -kernel ~/linux/arch/riscv/boot/Image \
  -initrd rootfs.cpio \
  -append "console=ttyS0 init=/init" \
  -smp 4 \
  -m 2G
