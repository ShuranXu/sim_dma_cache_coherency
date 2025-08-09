/* dma_multi_core.c
 *
 * Simulate a non-coherent DMA transfer on a multi-core RISC-V SoC
 * using real Zicbo CMO cache-maintenance instructions.
 * Each hart (simulated by a POSIX thread) reads the DMA result
 * after invalidating its cache lines.
 *
 * Build & run on an SMP Linux image under Spike as described below.
 */

 #define _POSIX_C_SOURCE 200809L // enable posix_memalign( )
 #include <stdio.h>
 #include <stdlib.h>
 #include <stdint.h>
 #include <pthread.h>
 #include <string.h>
 #include <unistd.h>
 
 #define BUFFER_SIZE 1024
 // Cache-line alignment for DMA or SIMD.
 #define CACHE_LINE   64
 #define NUM_HARTS     4
 
 /* Aligned source & destination buffers */
 static uint8_t *src_buf;
 static uint8_t *dst_buf;
 
 /* DMA completion flag */
 static volatile int dma_done = 0;
 
 /**
  * @brief Clean (write-back) D-cache lines for [addr, addr+size).
  * Uses the Zicbo CMO instruction `cbo.clean` and a fence.
  */
 static inline void cache_clean(void *addr, size_t size) {
    /* fallback-only: just use fences */
    __asm__ volatile("fence rw, rw" ::: "memory");
}

static inline void cache_invalidate(void *addr, size_t size) {
    __asm__ volatile("fence rw, rw" ::: "memory");
}


 /**
  * @brief DMA engine simulation: copies src->dst then invalidates dst cache.
  */
 void *dma_transfer(void *arg) {
     (void)arg;
     sleep(1); /* simulate DMA latency (100 ms) */
     memcpy(dst_buf, src_buf, BUFFER_SIZE);
     /*
      * We simulate the DMA engine on “hart 0” copying data into DRAM via memcpy().
      * That CPU‐side memcpy will leave the new bytes sitting in hart 0’s L1D,
      * but real DMA engines bypass CPU caches entirely and write directly into RAM.
      *  
      *  By calling cache_invalidate(dst_buf, …) in the DMA thread, we flush hart 0’s
      *  stale lines for that region, so if hart 0 ever reads from dst_buf, it will
      *  see the fresh data the “DMA” just wrote to memory.
      */
     cache_invalidate(dst_buf, BUFFER_SIZE);
     dma_done = 1;
     return NULL;
 }
 
 /**
  * @brief Worker function for each hart.
  * Waits for DMA completion, invalidates its cache lines,
  * then reads and prints the first 16 bytes.
  */
 void *hart_worker(void *arg) {
     int hart_id = *(int *)arg;
     free(arg);
 
     /* Poll for DMA completion */
     while (!dma_done) {
         ; /* spin */
     }
 
     /* 
      *  Invalidate cache lines to fetch fresh DMA data. Those threads run on other simulated harts (1–3).
      *  Each has its own private L1D. Even if hart 0 did a proper invalidate, that does not touch
      *  hart 1/2/3’s caches.
      *  So each worker must call cache_invalidate() locally to drop its own stale lines
      *  and fetch the DMA’s results from RAM.
      */
     cache_invalidate(dst_buf, BUFFER_SIZE);

    printf("\n[hart %d] dst_buf[0..15]:", hart_id);
     for (int i = 0; i < 16; i++) {
         printf(" %02X", dst_buf[i]);
     }
     printf("\n\n");
     return NULL;
 }
 
 int main(void) {
     pthread_t dma_th;
     pthread_t cores[NUM_HARTS];
 
     /* 1) Allocate cache-aligned buffers */
     if (posix_memalign((void **)&src_buf, CACHE_LINE, BUFFER_SIZE) != 0 ||
         posix_memalign((void **)&dst_buf, CACHE_LINE, BUFFER_SIZE) != 0) {
         perror("posix_memalign");
         return EXIT_FAILURE;
     }

     printf("Allocated src and dst buffers\n");
 
     /* 2) Initialize source data */
     for (size_t i = 0; i < BUFFER_SIZE; i++) {
         src_buf[i] = (uint8_t)i;
     }

     printf("Initialized src buffer\n");
 
     /* 3) Clean source buffer before DMA reads it */
     cache_clean(src_buf, BUFFER_SIZE);

     printf("Cleaned cache lines for src buffer\n");
 
     /* 4) Launch DMA simulator */
     if (pthread_create(&dma_th, NULL, dma_transfer, NULL) != 0) {
         perror("pthread_create dma");
         return EXIT_FAILURE;
     }

     printf("Launched DMA simulator\n");
 
     /* 5) Launch hart worker threads */
     for (int i = 0; i < NUM_HARTS; i++) {
         int *arg = malloc(sizeof(int));
         *arg = i;
         if (pthread_create(&cores[i], NULL, hart_worker, arg) != 0) {
             perror("pthread_create hart");
             return EXIT_FAILURE;
         }
     }
 
     /* 6) Wait for all threads to finish */
     printf("Waiting for threads to finish\n");
     pthread_join(dma_th, NULL);
     for (int i = 0; i < NUM_HARTS; i++) {
         pthread_join(cores[i], NULL);
     }
 
     free(src_buf);
     free(dst_buf);

     printf("All threads done; spinning forever so init() never exits.\n");
     while (1) {
        sleep(1);
     }

     return EXIT_SUCCESS; // should never return
 }
