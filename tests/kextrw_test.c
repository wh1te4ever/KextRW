#include <libkextrw.h>

#include <mach-o/loader.h>
#include <mach/arm/vm_param.h>
#include <mach/mach_init.h>
#include <mach/thread_act.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

// xnu-8019.41.5/osfmk/kern/zalloc.h
#define __options_decl(_name, _type, ...) \
	        typedef enum : _type __VA_ARGS__ __enum_open __enum_options _name

__options_decl(zalloc_flags_t, uint32_t, {
	// values smaller than 0xff are shared with the M_* flags from BSD MALLOC
	Z_WAITOK        = 0x0000,
	Z_NOWAIT        = 0x0001,
	Z_NOPAGEWAIT    = 0x0002,
	Z_ZERO          = 0x0004,

	Z_NOFAIL        = 0x8000,

	/* convenient c++ spellings */
	Z_NOWAIT_ZERO          = Z_NOWAIT | Z_ZERO,
	Z_WAITOK_ZERO          = Z_WAITOK | Z_ZERO,
	Z_WAITOK_ZERO_NOFAIL   = Z_WAITOK | Z_ZERO | Z_NOFAIL, /* convenient spelling for c++ */
	Z_NOZZC         = 0x4000,

	/** used by kalloc to propagate vm tags for -zt */
	Z_VM_TAG_MASK   = 0xffff0000,

#define Z_VM_TAG_SHIFT        16
#define Z_VM_TAG(tag)         ((zalloc_flags_t)(tag) << Z_VM_TAG_SHIFT)
});

#define KALLOC_DATA_EXTERNAL    kslide(0xFFFFFE0007244D08)
#define KFREE_DATA_EXTERNAL     kslide(0xFFFFFE00072452D8)

void khexdump(uint64_t addr, size_t size) {
    void *data = malloc(size);
    kreadbuf(addr, data, size);
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        if ((i % 16) == 0)
        {
            printf("[0x%016llx+0x%03zx] ", addr, i);
//            printf("[0x%016llx] ", i + addr);
        }
        
        printf("%02X ", ((unsigned char*)data)[i]);
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            printf(" ");
            if ((i+1) % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
    free(data);
}

int main(void) {
    if (kextrw_init() == -1) {
        printf("Failed to initialize KextRW\n");
        return 1;
    }

    uint64_t kernelBase = get_kernel_base();

    printf("Kernel base (VA): 0x%llX\n", kernelBase);
    printf("Kernel base (PA): 0x%llX\n", kvtophys(kernelBase));
    printf("Kernel slide: 0x%llX\n", kslide(0));

    uint64_t kptr = kcall(KALLOC_DATA_EXTERNAL, (uint64_t []){ 0x100, Z_WAITOK}, 2);
    printf("kcall: kalloc_data_external(0x100) -> 0x%llX\n", kptr);

    kwrite64(kptr, 0x13371337cafebabe);
    khexdump(kptr, 0x100);

    if (kptr) {
        kcall(KFREE_DATA_EXTERNAL, (uint64_t []){ kptr, 0x100 }, 2);
    }

    kextrw_deinit();
    return 0;
}