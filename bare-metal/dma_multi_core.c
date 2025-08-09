/* dma_multi_core_bare_metal.c
 *
 * Bare-metal multi-hart DMA simulation on RISC-V using SBI console for output.
 * No POSIX threads or heap; runs under Spike + pk with newlib.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define BUFFER_SIZE     1024
#define CACHE_LINE      64
#define NUM_HARTS       4
// MMIO address of the virt‐platform UART0 in Spike
#define UART0           0x10000000UL

// The following two globals are part of Spike’s “host–target” (semi-hosting) protocol:
// - tohost is a word our bare-metal program writes into when it wants to send a command
// or data to the simulator (for example, “please exit with this status code”).
// - fromhost is where the simulator can deposit data or signals back into our program.
volatile uint64_t tohost  __attribute__((section(".tohost")))  = 0;
volatile uint64_t fromhost __attribute__((section(".fromhost"))) = 0;

/* Aligned source & destination buffers */
static uint8_t src_buf[BUFFER_SIZE] __attribute__((aligned(CACHE_LINE)));
static uint8_t dst_buf[BUFFER_SIZE] __attribute__((aligned(CACHE_LINE)));

/* DMA completion flag (visible to all harts) */
static volatile int dma_done = 0;

static const char *hexdigits = "0123456789ABCDEF";


// static inline void sbi_console_putchar(int ch) {
//   /*  
//    * a0 holds the character to print, and a7 holds the SBI extension ID.
//    * SBI spec defines extension ID 1 as “console_putchar”. 
//    */
//   register uintptr_t a0 asm("a0") = ch;
//   register uintptr_t a7 asm("a7") = 1;
//   /*  
//    * Trigger an environment call (ecall) in supervisor mode,
//    * which the SBI (or emulator proxy) catches and services.
//    */
//   asm volatile("ecall" :: "r"(a0), "r"(a7) : "memory");
// }

// Raw put-char to the UART
static inline void uart_putchar(char c) {
    *(volatile char *)UART0 = c;
}

// Write a NUL-terminated string
static inline void console_puts(const char *s) {
  while (*s) uart_putchar(*s++);
}

 static inline void cache_clean(void *addr, size_t size) {
     uintptr_t p    = (uintptr_t)addr & ~(CACHE_LINE - 1);
     uintptr_t end  = (uintptr_t)addr + size;
     for (; p < end; p += CACHE_LINE) {
        // cbo.clean performs a clean of the cache block at the effective
        // address in the base register: if the block is dirty, its modified
        // data are written back to memory (and any higher-level caches) but
        // the block remains valid in the cache hierarchy (no invalidation).
        #if defined(__riscv_zicbom)
        // The parentheses around %0 are standard RISC-V syntax for a memory operand:
        // they tell the assembler “use the register %0 as a pointer, and apply
        // cbo.clean at that effective address.”
            __asm__ volatile("cbo.clean (%0)" :: "r"(p) : "memory");
        #else
            __asm__ volatile("fence rw, rw" ::: "memory");  // fallback: order only
        #endif
     }
     __asm__ volatile("fence rw, rw" ::: "memory");
 }
 
 /**
  * @brief Invalidate D-cache lines for [addr, addr+size).
  * Uses the Zicbo CMO instruction `cbo.inval` and a fence.
  */
 static inline void cache_invalidate(void *addr, size_t size) {
     uintptr_t p    = (uintptr_t)addr & ~(CACHE_LINE - 1);
     uintptr_t end  = (uintptr_t)addr + size;
     for (; p < end; p += CACHE_LINE) {
        // If a cache line corresponding to that address is present
        // in the coherent caches, it is invalidated (i.e. de-allocated),
        // and any dirty data in the line is discarded (no write-back)
        #if defined(__riscv_zicbom)
        __asm__ volatile("cbo.inval (%0)" :: "r"(p) : "memory");
        #else
            __asm__ volatile("fence rw, rw" ::: "memory");  // fallback: order only
        #endif
     }
     __asm__ volatile("fence rw, rw" ::: "memory");
 }


/* Read mhartid CSR */
static inline int read_hartid(void) {
    int hart;
    __asm__ volatile("csrr %0, mhartid" : "=r"(hart));
    return hart;
}


/**
 * @brief Entry point for bare-metal multi-hart DMA simulation on RISC-V.
 *
 * This function reads the current hart ID and prints a startup message.
 * - Hart 0:
 *   - Initializes the source buffer and performs a simulated DMA transfer.
 *   - Cleans and invalidates caches around the transfer.
 *   - Signals completion via an atomic release store.
 *   - Prints a "DMA done" message.
 * - Other Harts (1..3):
 *   - Wait for the DMA completion using an atomic acquire load.
 *   - Invalidate their caches for the destination buffer.
 *   - Print the first 16 bytes of the transferred data in hexadecimal.
 *
 * After all output, the function writes to the `tohost` symbol to request
 * Spike to terminate, then enters an infinite WFI loop and never returns.
 *
 * @return Does not return under normal execution. After signaling tohost,
 *         the hart enters low-power wait-for-interrupt state indefinitely.
 */
int main(void) {

    int hart = read_hartid();

     console_puts("[hart ");
     uart_putchar('0' + hart);      // only works 0–3
     console_puts("] starting\n");

    if (hart == 0) {

        console_puts("[hart ");
        uart_putchar('0' + hart);
        console_puts("] initialize source and start DMA\n");

        /* Hart 0: initialize source and start DMA */
        for (size_t i = 0; i < BUFFER_SIZE; i++) {
            src_buf[i] = (uint8_t)i;
        }

        /* Write-back src buffer to memory */
        cache_clean(src_buf, BUFFER_SIZE);

        /* Simulate DMA latency */
        for (volatile int i = 0; i < 1000000; i++);

        /* Perform DMA copy */
        memcpy(dst_buf, src_buf, BUFFER_SIZE);
        cache_invalidate(dst_buf, BUFFER_SIZE);

        /* Signal completion */
        __atomic_store_n(&dma_done, 1, __ATOMIC_RELEASE);

        console_puts("[hart ");
        uart_putchar('0' + hart);
        console_puts("] DMA done\n");

    } else {

        console_puts("[hart ");
        uart_putchar('0' + hart);
        console_puts("] wait for DMA \n");

        /* Other harts: wait for DMA */
        while (!__atomic_load_n(&dma_done, __ATOMIC_ACQUIRE)) {
            /* spin */
        }
        /* Invalidate cache and print result */
        cache_invalidate(dst_buf, BUFFER_SIZE);
        console_puts("[hart ");
        uart_putchar('0' + hart);
        console_puts("] dst_buf[0..15]:\n");

        for (int i = 0; i < 16; i++) {
            char buf[4];
            buf[0] = ' ';
            buf[1] = hexdigits[(dst_buf[i] >> 4) & 0xF];
            buf[2] = hexdigits[ dst_buf[i]       & 0xF];
            buf[3] = '\0';
            console_puts(buf);
        }
        console_puts("\n");
    }
    
    // signal to Spike “we’re done, exit with code 0”
    // tohost = 1;
    /* Prevent hart from exiting */
    while (1) {
        __asm__ volatile("wfi");
    }

    return 0; // unreachable
}
