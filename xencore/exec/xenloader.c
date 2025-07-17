#include <string.h>

#include <xencore/exec/xenloader.h>
#include <xencore/xenmem/xenalloc.h>
#include <xencore/xenio/tty.h>

Elf64 *load_elf64(void* elf_data) {
    if (!elf_data) return NULL;

    // Allocate memory for the ELF structure and copy the header
    Elf64 *elf = xen_alloc(sizeof(Elf64));
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)elf_data;
    elf->header = *ehdr;

    // Validate ELF magic number
    if (memcmp(ehdr->e_ident, "\x7F" "ELF", 4) != 0) {
#ifdef HLOS_DEBUG
        tty_printf("[XenLoader] Invalid ELF magic number\n");
#endif
        xen_free(elf);
        return NULL;
    }

    // Executable or DYN
    if (ehdr->e_type != 2 && ehdr->e_type != 3) {
#ifdef HLOS_DEBUG
        tty_printf("[XenLoader] Unsupported ELF type: %d\n", ehdr->e_type);
#endif
        xen_free(elf);
        return NULL;
    } 

#ifdef ARCH_x86_64
    // 64-bit
    if (ehdr->e_ident[4] != 2) {
#ifdef HLOS_DEBUG
        tty_printf("[XenLoader] Unsupported ELF class: %d\n", ehdr->e_ident[4]);
#endif
        xen_free(elf);
        return NULL;
    }

    // x86-64
    if (ehdr->e_machine != 0x3E) {
#ifdef HLOS_DEBUG
        tty_printf("[XenLoader] Unsupported ELF machine: %d\n", ehdr->e_machine);
#endif
        xen_free(elf);
        return NULL;
    }
#endif

    // Allocate memory for the segments
    Elf64_Phdr* phdrs = (Elf64_Phdr*)((uint8_t*)elf_data + ehdr->e_phoff);
    elf->segments = xen_alloc(sizeof(Elf64_Phdr) * ehdr->e_phnum);
    if (!elf->segments) {
#ifdef HLOS_DEBUG
        tty_printf("[XenLoader] Failed to allocate memory for ELF segments\n");
#endif
        xen_free(elf);
        return NULL;
    }

    // Load each segment into memory
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        Elf64_Phdr* ph = &phdrs[i];
        if (ph->p_type != PT_LOAD)
            continue;

        uint64_t segment_memsz = ph->p_memsz;
        uint64_t segment_filesz = ph->p_filesz;
        void* segment_data = (uint8_t*)elf_data + ph->p_offset;

        // Allocate and zero memory (aligned to page size)
        void* dest = xen_alloc_aligned(segment_memsz);
        if (dest) {
            memset(dest, 0, segment_memsz);
            memcpy(dest, segment_data, segment_filesz);
            ph->p_paddr = (uint64_t)dest; // Store the physical address of the segment
            continue;
        }

        // If allocation fails, free the previously allocated segments and return NULL
#ifdef HLOS_DEBUG
        tty_printf("[XenLoader] Failed to allocate memory for ELF segment %d\n", i);
#endif

        for (int j = 0; j < i; ++j) {
            Elf64_Phdr* prev_ph = &elf->segments[j];
            if (prev_ph->p_type == PT_LOAD) {
                void* prev_segment_paddr = (void*)(prev_ph->p_paddr);
                xen_free(prev_segment_paddr);
            }
        }

        xen_free(elf->segments);
        xen_free(elf);
        return NULL;
    }

    // Allocate memory for the sections
    Elf64_Shdr* shdrs = (Elf64_Shdr*)((uint8_t*)elf_data + ehdr->e_shoff);
    elf->sections = xen_alloc(sizeof(Elf64_Shdr) * ehdr->e_shnum);

    if (!elf->sections) {
#ifdef HLOS_DEBUG
        tty_printf("[XenLoader] Failed to allocate memory for ELF sections\n");
#endif

        // Free the segments if they were allocated
        for (int i = 0; i < ehdr->e_phnum; ++i) {
            Elf64_Phdr* ph = &elf->segments[i];
            if (ph->p_type == PT_LOAD) {
                void* segment_paddr = (void*)(ph->p_paddr);
                xen_free(segment_paddr);
            }
        }

        xen_free(elf->segments);
        xen_free(elf);
        return NULL;
    }

    memcpy(elf->sections, shdrs, sizeof(Elf64_Shdr) * ehdr->e_shnum);
    memcpy(elf->segments, phdrs, sizeof(Elf64_Phdr) * ehdr->e_phnum);
    return elf;
}

void free_elf64(Elf64 *elf) {
    if (!elf) return;

#ifdef HLOS_DEBUG
    tty_printf("[XenLoader] Freeing ELF @ 0x%x\n", (uint64_t)elf);
#endif

    if (!elf->segments) {
        xen_free(elf);
        return;
    }

    for (int i = 0; i < elf->header.e_phnum; ++i) {
        Elf64_Phdr* ph = &elf->segments[i];
        if (ph->p_type == PT_LOAD) {
            // Free the segment memory if it was allocated
            void* segment_paddr = (void*)(ph->p_paddr);
            xen_free(segment_paddr);
        }
    }

    xen_free(elf->segments);
    xen_free(elf);
}
