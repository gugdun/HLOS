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

#ifdef ARCH_x86_64
#include <xencore/arch/x86_64/fpu.h>
#include <xencore/arch/x86_64/tss.h>
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
#include <xencore/hazardous/xenloader.h>
#include <xencore/hazardous/environment.h>
#include <xencore/timer/sleep.h>
#include <xencore/gman/gman.h>

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
    vfs_node_t *elf_file = vfs_lookup("/test_sample/test.elf");
    if (elf_file) {
        tty_printf("[Kernel] Found ELF file: %s\n", "/test_sample/test.elf");
        Elf64 *elf = load_elf64(elf_file->file.data);
        tty_printf("[Kernel] Setting up hazardous environment...\n");
        struct HazardousContext *ctx = setup_hazardous_environment(elf);
        tty_printf("[Kernel] Going to enter hazardous environment...\n");
        enter_hazardous_environment(ctx);
    }

    struct DemoTriangleState state = demo_triangle_init();
    while (1) demo_triangle_tick(&state);
}
