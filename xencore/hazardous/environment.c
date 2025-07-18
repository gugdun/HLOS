#ifdef ARCH_x86_64
#include <xencore/arch/x86_64/paging.h>
#include <xencore/arch/x86_64/segments.h>
#endif

#include <xencore/hazardous/environment.h>
#include <xencore/xenmem/xenalloc.h>
#include <xencore/xenio/tty.h>

struct HazardousContext *setup_hazardous_environment(Elf64 *elf)
{
    struct HazardousContext *ctx = xen_alloc(sizeof(struct HazardousContext));
#ifdef ARCH_x86_64
    ctx->page_table = create_user_pml4();
#endif
    ctx->entry_point = elf->header.e_entry;

    // Map ELF segments
    for (size_t i = 0; i < elf->header.e_phnum; ++i) {
        Elf64_Phdr *phdr = &elf->segments[i];
        tty_printf(
            "[Hazardous] Segment %u: type=%u, vaddr=0x%x, paddr=0x%x, memsz=%u\n",
            i, phdr->p_type, phdr->p_vaddr, phdr->p_paddr, phdr->p_memsz
        );

        if (phdr->p_type == PT_LOAD) {
            phdr->p_paddr = virt_to_phys(phdr->p_paddr);
            tty_printf(
                "[Hazardous] Mapping segment %u: virt 0x%x -> phys 0x%x, size %u\n",
                i, phdr->p_vaddr, phdr->p_paddr, phdr->p_memsz
            );
            map_user_segment(ctx->page_table, phdr->p_vaddr, phdr->p_paddr, phdr->p_memsz);
        }
    }

    // Allocate and map user stack
    uint64_t stack_bottom = USER_STACK_TOP - USER_STACK_SIZE;
    uint64_t phys = virt_to_phys((uint64_t)xen_alloc_aligned(USER_STACK_SIZE));
    map_range(ctx->page_table, stack_bottom, phys, USER_STACK_SIZE, PAGE_RW | PAGE_USER);
    ctx->stack_top = USER_STACK_TOP;

    return ctx;
}

__attribute__((noreturn)) void enter_hazardous_environment(struct HazardousContext *ctx)
{
    uint64_t user_entry = ctx->entry_point;
    uint64_t user_stack = ctx->stack_top;

#ifdef ARCH_x86_64
    uint64_t cr3_phys = virt_to_phys((uint64_t)ctx->page_table);

    uint64_t rflags;
    __asm__ volatile ("pushfq; pop %0" : "=r"(rflags));
    rflags |= 1ULL << 9;

    tty_printf(
        "[Hazardous] entry=0x%x stack=0x%x cr3=0x%x\n",
        (void*)user_entry, (void*)user_stack, (void*)cr3_phys
    );

    load_pml4((uint64_t *)cr3_phys);

    __asm__ volatile (
        "cli\n\t"
        "mov %[uds], %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "pushq %[uds]\n\t"
        "pushq %[stk]\n\t"
        "pushq %[rfl]\n\t"
        "pushq %[ucs]\n\t"
        "pushq %[rip]\n\t"
        "iretq\n\t"
        :
        : [uds]"i"(USER_DS),
          [ucs]"i"(USER_CS),
          [stk]"r"(user_stack),
          [rip]"r"(user_entry),
          [rfl]"r"(rflags)
        : "rax", "memory"
    );
#endif

    __builtin_unreachable();
}
