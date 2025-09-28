#ifndef LIBKEXTRW_H
#define LIBKEXTRW_H

#include <stdint.h>
#include <mach/mach.h>

extern uint64_t gKernelBase, gKernelSlide;

// gKernelSlide matches the kernel slide in a panic log,
// but we need to use the KernelCache slide, which is
// always 0x110000 less than the normal kernel slide.
#define kslide(x) (x + gKernelSlide - 0x110000)

/* Initialisation and deinitialisation */
int kextrw_init(void);
void kextrw_deinit(void);

/* Virtual read/write */
uint8_t kread8(uint64_t addr);
uint16_t kread16(uint64_t addr);
uint32_t kread32(uint64_t addr);
uint64_t kread64(uint64_t addr);
int kreadbuf(uint64_t addr, void *buf, size_t len);

void kwrite8(uint64_t addr, uint8_t val);
void kwrite16(uint64_t addr, uint16_t val);
void kwrite32(uint64_t addr, uint32_t val);
void kwrite64(uint64_t addr, uint64_t val);
int kwritebuf(uint64_t addr, void *buf, size_t len);

uint64_t kreadptr(uint64_t addr);
uint64_t kreadptr_smr(uint64_t addr);

/* Physical read/write */
uint8_t physread8(uint64_t addr);
uint16_t physread16(uint64_t addr);
uint32_t physread32(uint64_t addr);
uint64_t physread64(uint64_t addr);
int physreadbuf(uint64_t addr, void *buf, size_t len);

void physwrite8(uint64_t addr, uint8_t val);
void physwrite16(uint64_t addr, uint16_t val);
void physwrite32(uint64_t addr, uint32_t val);
void physwrite64(uint64_t addr, uint64_t val);
int physwritebuf(uint64_t addr, void *buf, size_t len);

/* Kernel call */
uint64_t kcall(uint64_t fn, uint64_t *args, uint32_t argsCnt);

/* Translation */
uint64_t kvtophys(uint64_t va);

/* Allocations */
uint64_t kalloc(uint64_t size);
void kfree(uint64_t addr, uint64_t size);

/* Utilities */
uint64_t get_kernel_base();

#endif // LIBKEXTRW_H