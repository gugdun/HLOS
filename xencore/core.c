/*-------------------------------------------------------------------------------------*\
|                                                                                       |
|    ██████╗ ██╗      █████╗  ██████╗██╗  ██╗    ███╗   ███╗███████╗███████╗ █████╗     |
|    ██╔══██╗██║     ██╔══██╗██╔════╝██║ ██╔╝    ████╗ ████║██╔════╝██╔════╝██╔══██╗    |
|    ██████╔╝██║     ███████║██║     █████╔╝     ██╔████╔██║█████╗  ███████╗███████║    |
|    ██╔══██╗██║     ██╔══██║██║     ██╔═██╗     ██║╚██╔╝██║██╔══╝  ╚════██║██╔══██║    |
|    ██████╔╝███████╗██║  ██║╚██████╗██║  ██╗    ██║ ╚═╝ ██║███████╗███████║██║  ██║    |
|    ╚═════╝ ╚══════╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝    ╚═╝     ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝    |
|                                                                                       |
|    Black Mesa Operating Environment – Core Module                                     |
|    Confidential ████-Level Access Required                                            |
|                                                                                       |
\*-------------------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef ARCH_x86_64
#include <xencore/arch/x86_64/fpu.h>
#include <xencore/arch/x86_64/gdt.h>
#include <xencore/arch/x86_64/idt.h>
#include <xencore/arch/x86_64/pic.h>
#include <xencore/arch/x86_64/pit.h>
#include <xencore/arch/x86_64/paging.h>
#endif

#include <xencore/common.h>
#include <xencore/xenio/serial.h>
#include <xencore/xenio/tty.h>
#include <xencore/graphics/framebuffer.h>
#include <xencore/xenmem/xenmap.h>
#include <xencore/xenfs/vfs.h>
#include <xencore/xenfs/test_sample.h>
#include <xencore/timer/sleep.h>
#include <xencore/exec/xenloader.h>

#include <demo/triangle.h>

void resonance_cascade(struct FramebufferParams fb_params, struct TestSampleParams sample_params, struct MemoryMapParams memmap_params) {
    serial_init();
    fb_init(&fb_params);

#ifdef ARCH_x86_64
    enable_fpu_sse();
    disable_interrupts();
    setup_tss();
    setup_gdt();
    setup_idt();
    setup_paging(&memmap_params, fb_params.base, fb_params.size);
    remap_pic();
    setup_pit(100);
    enable_interrupts();
#endif

    xenmap_init();
    vfs_init();
    analyse_test_sample(&sample_params);

    // Test loading an ELF executable
    const char *executable_path = "/test_sample/test.elf";
    vfs_node_t *node = vfs_lookup(executable_path);

    if (node && node->type == VFS_NODE_FILE) {
        tty_printf("[Core] Loading ELF from %s...\n", executable_path);
        Elf64 *elf = load_elf64(node->file.data);

        if (!elf) {
            tty_printf("[Core] Failed to load ELF!\n");
        } else {
            tty_printf("[Core] ELF loaded successfully! Entry: 0x%x\n", elf->header.e_entry);

            // Identity map the ELF segments
            for (int i = 0; i < elf->header.e_phnum; ++i) {
                Elf64_Phdr *ph = &elf->segments[i];
                tty_printf("[Core] Segment %d: Type: %d, VAddr: 0x%x, PAddr: 0x%x\n", i, ph->p_type, ph->p_vaddr, ph->p_paddr);

                if (ph->p_type == PT_LOAD) {
                    struct MemoryMapEntry entry = {
                        KernelCode,
                        0,
                        ph->p_vaddr,
                        ph->p_vaddr,
                        (ph->p_memsz + PAGE_SIZE_4KB - 1) / PAGE_SIZE_4KB,
                        0
                    };
                    map_identity(&entry);

                    // Copy the segment data to the mapped address
                    void *dest = (void *)ph->p_vaddr;
                    memcpy(dest, (void *)ph->p_paddr, ph->p_filesz);
                }
            }

            // Jump to the entry point
            void (*entry_point)(void) = (void (*)(void))elf->header.e_entry;
            tty_printf("[Core] Jumping to entry point at 0x%x...\n", (uint64_t)entry_point);
            entry_point();
            tty_printf("[Core] Executable returned!\n");
        }
    }

    struct DemoTriangleState state = demo_triangle_init();
    while (1) demo_triangle_tick(&state);
}
