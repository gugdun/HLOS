#include <stdint.h>
#include <stddef.h>

#include <xencore/arch/x86_64/paging.h>

#include <xencore/xenio/tty.h>
#include <xencore/common.h>

#define ALIGN_DOWN(x, a) ((uint64_t)(x) & ~((uint64_t)(a) - 1))
#define ALIGN_UP(x, a)   ((((uint64_t)(x) + ((uint64_t)(a) - 1)) & ~((uint64_t)(a) - 1)))
#define ALIGN_DOWN_4K(x) ALIGN_DOWN((x), PAGE_SIZE_4KB)
#define ALIGN_UP_4K(x)   ALIGN_UP((x),   PAGE_SIZE_4KB)
#define ALIGN_DOWN_2M(x) ALIGN_DOWN((x), PAGE_SIZE_2MB)
#define ALIGN_UP_2M(x)   ALIGN_UP((x),   PAGE_SIZE_2MB)

__attribute__((aligned(PAGE_SIZE_4KB))) uint64_t kernel_pml4[512];

uint64_t next_virtual_heap_addr = VIRT_HEAP_BASE;

static uint8_t *early_alloc_buffer = NULL;
static size_t early_alloc_size = 0;
static size_t early_alloc_offset = 0;

static uint64_t *get_or_create_table(uint64_t *parent, size_t index, uint64_t flags)
{
    uint64_t entry = parent[index];

    if (!(entry & PAGE_PRESENT)) {
        uint64_t *new_table = early_alloc_page();
        for (int i = 0; i < 512; ++i) new_table[i] = 0;
        entry = ((uint64_t)new_table) | PAGE_PRESENT;
        if (flags & PAGE_RW)   entry |= PAGE_RW;
        if (flags & PAGE_USER) entry |= PAGE_USER;
    } else {
        if (flags & PAGE_USER) entry |= PAGE_USER;   // TEMP: promote
        if (flags & PAGE_RW)   entry |= PAGE_RW;     // TEMP: promote
    }

    parent[index] = entry;
    return (uint64_t *)(entry & ~0xFFFULL);
}

// Map single 4K page
static void map_page_4k(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags)
{
#ifdef HLOS_DEBUG
    if ((virt & (uint64_t)(PAGE_SIZE_4KB - 1)) || (phys & (uint64_t)(PAGE_SIZE_4KB - 1))) {
        tty_printf("[Paging] WARN: 4K map unaligned v=0x%x p=0x%x\n", (void*)virt, (void*)phys);
    }
#endif
    size_t pml4_index = (virt >> 39) & 0x1FF;
    size_t pdpt_index = (virt >> 30) & 0x1FF;
    size_t pd_index   = (virt >> 21) & 0x1FF;
    size_t pt_index   = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = get_or_create_table(pml4, pml4_index, flags);
    uint64_t *pd   = get_or_create_table(pdpt, pdpt_index, flags);
    uint64_t *pt   = get_or_create_table(pd, pd_index, flags);

    pt[pt_index] = phys | (flags & ~(PAGE_PS)) | PAGE_PRESENT;
}

// Map single 2M page
static void map_page_2mb(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags)
{
#ifdef HLOS_DEBUG
    if ((virt & (uint64_t)(PAGE_SIZE_2MB - 1)) || (phys & (uint64_t)(PAGE_SIZE_2MB - 1))) {
        tty_printf("[Paging] WARN: 2M map unaligned v=0x%x p=0x%x\n", (void*)virt, (void*)phys);
    }
#endif
    size_t pml4_index = (virt >> 39) & 0x1FF;
    size_t pdpt_index = (virt >> 30) & 0x1FF;
    size_t pd_index   = (virt >> 21) & 0x1FF;

    uint64_t *pdpt = get_or_create_table(pml4, pml4_index, flags);
    uint64_t *pd   = get_or_create_table(pdpt, pdpt_index, flags);

    pd[pd_index] = phys | (flags | PAGE_PS) | PAGE_PRESENT;
}

uint64_t virt_to_phys(uint64_t virt)
{
    const uint64_t VA = virt;

    size_t pml4_index = (VA >> 39) & 0x1FF;
    size_t pdpt_index = (VA >> 30) & 0x1FF;
    size_t pd_index   = (VA >> 21) & 0x1FF;
    size_t pt_index   = (VA >> 12) & 0x1FF;

    /* --- PML4 --- */
    uint64_t pml4e = kernel_pml4[pml4_index];
    if (!(pml4e & PAGE_PRESENT)) return 0;

    uint64_t *pdpt = (uint64_t *)(pml4e & ~0xFFFULL);

    /* --- PDPT --- */
    uint64_t pdpte = pdpt[pdpt_index];
    if (!(pdpte & PAGE_PRESENT)) return 0;

    // if (pdpte & PAGE_PS) {
    //     /* 1GiB page */
    //     uint64_t phys_base = pdpte & ~((uint64_t)PAGE_SIZE_1GB - 1);
    //     return phys_base + (VA & (PAGE_SIZE_1GB - 1));
    // }

    uint64_t *pd = (uint64_t *)(pdpte & ~0xFFFULL);

    /* --- PD --- */
    uint64_t pde = pd[pd_index];
    if (!(pde & PAGE_PRESENT)) return 0;

    if (pde & PAGE_PS) {
        /* 2MiB page */
        uint64_t phys_base = pde & ~((uint64_t)PAGE_SIZE_2MB - 1);
        return phys_base + (VA & (PAGE_SIZE_2MB - 1));
    }

    uint64_t *pt = (uint64_t *)(pde & ~0xFFFULL);

    /* --- PT --- */
    uint64_t pte = pt[pt_index];
    if (!(pte & PAGE_PRESENT)) return 0;

    uint64_t phys_base = pte & ~((uint64_t)PAGE_SIZE_4KB - 1);
    return phys_base + (VA & (PAGE_SIZE_4KB - 1));
}

void *early_alloc_page(void)
{
    if (early_alloc_offset + PAGE_SIZE_4KB > early_alloc_size) {
        tty_printf("[Paging] Early allocation buffer exhausted!\n");
        while (1) { halt(); }
    }
    void *ptr = &early_alloc_buffer[early_alloc_offset];
    early_alloc_offset += PAGE_SIZE_4KB;
    return (void *)ALIGN_UP_4K((uint64_t)ptr);
}

void load_pml4(uint64_t *pml4)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4) : "memory");
}

uint64_t *create_user_pml4(void)
{
    // allocate a new PML4 for user space
    uint64_t *user_pml4 = early_alloc_page();
    for (int i = 0; i < 512; i++) {
        user_pml4[i] = 0;
    }

    for (int i = 0; i < 512; i++) {
        if (kernel_pml4[i] & 0x1) { // Present
            user_pml4[i] = kernel_pml4[i];
        }
    }

    return user_pml4;
}

// Hybrid mapper: uses 2M pages where possible, 4K for leftovers
void map_range(uint64_t *pml4, uint64_t virt_start, uint64_t phys_start, uint64_t size, uint64_t flags)
{
    uint64_t virt = virt_start;
    uint64_t phys = phys_start;
    uint64_t end  = phys_start + size;

    while (phys < end) {
        // If phys is aligned to 2MB, use 2MB mapping
        if (!(phys & (uint64_t)(PAGE_SIZE_2MB - 1)) && !(virt & (uint64_t)(PAGE_SIZE_2MB - 1)) && (end - phys) >= PAGE_SIZE_2MB) {
            map_page_2mb(pml4, virt, phys, flags);
            virt += PAGE_SIZE_2MB;
            phys += PAGE_SIZE_2MB;
        }
        // Otherwise, use 4K mapping
        else {
            map_page_4k(pml4, virt, phys, flags);
            virt += PAGE_SIZE_4KB;
            phys += PAGE_SIZE_4KB;
        }
    }
}

void map_user_segment(uint64_t *user_pml4, uint64_t virt, uint64_t phys, uint64_t size)
{
    map_range(user_pml4, virt, phys, size, PAGE_RW | PAGE_USER);
}

void map_identity(struct MemoryMapEntry *entry)
{
    uint64_t phys_start = entry->physical_start;
    uint64_t size_bytes = entry->size_pages * PAGE_SIZE_4KB;

    map_range(kernel_pml4, phys_start, phys_start, size_bytes, PAGE_RW);
#ifdef HLOS_DEBUG
    tty_printf(
        "[Paging] Identity mapped %u KiB @ 0x%x\n",
        entry->size_pages * 4, (uint64_t)phys_start
    );
#endif
}

size_t map_virtual(struct MemoryMapEntry *entry)
{
    uint64_t phys_start = entry->physical_start;
    uint64_t virt_start = entry->virtual_start;
    uint64_t size_bytes = entry->size_pages * PAGE_SIZE_4KB;

    map_range(kernel_pml4, virt_start, phys_start, size_bytes, PAGE_RW);
#ifdef HLOS_DEBUG
    tty_printf(
        "[Paging] Mapped %u KiB: virt 0x%x -> phys 0x%x\n",
        entry->size_pages * 4, (uint64_t)virt_start, (uint64_t)phys_start
    );
#endif
    return size_bytes;
}

void setup_paging(struct MemoryMapParams *params, uint64_t fb_base, size_t fb_size)
{
    // Find conventional memory size
    size_t conventional_memory_size = 0;
    for (size_t i = 0; i < params->memory_map_size; i += params->descriptor_size) {
        struct MemoryMapEntry *entry = (struct MemoryMapEntry *)((uint8_t *)params->memory_map + i);
        if (entry->type == ConventionalMemory) {
            conventional_memory_size += entry->size_pages * PAGE_SIZE_4KB;
        }
    }

    // Find memory for early allocator
    size_t alloc_start = 0;
    size_t alloc_size = conventional_memory_size / 2048;
    for (size_t i = 0; i < params->memory_map_size; i += params->descriptor_size) {
        struct MemoryMapEntry *entry = (struct MemoryMapEntry *)((uint8_t *)params->memory_map + i);
        if (entry->type == ConventionalMemory && entry->size_pages * PAGE_SIZE_4KB >= alloc_size && !(entry->physical_start & (uint64_t)(PAGE_SIZE_2MB - 1))) {
            alloc_start = ALIGN_UP_4K(entry->physical_start);
            entry->physical_start += alloc_size;
            entry->size_pages -= alloc_size / PAGE_SIZE_4KB;
            break;
        }
    }
    
    if (alloc_start == 0) {
        tty_printf("[Paging] No suitable memory found for early allocation buffer!\n");
        while (1) { halt(); }
    }

    tty_printf("[Paging] Allocated %u 4KiB pages for early allocator\n", alloc_size / PAGE_SIZE_4KB);

    early_alloc_buffer = (uint8_t *)alloc_start;
    early_alloc_size = alloc_size;
    early_alloc_offset = 0;

    // Identity map early allocation buffer
    struct MemoryMapEntry alloc_entry = {
        .type = ConventionalMemory,
        .pad = 0,
        .physical_start = alloc_start,
        .virtual_start = alloc_start,
        .size_pages = early_alloc_size / PAGE_SIZE_4KB,
        .attribute = 0
    };
    map_identity(&alloc_entry);

    // Identity map framebuffer
    size_t fb_pages = (fb_size / PAGE_SIZE_4KB) + 1;
    struct MemoryMapEntry fb_entry = { 0, 0, fb_base, fb_base, fb_pages, 0 };
    map_identity(&fb_entry);

    // Map all UEFI entries
    for (size_t i = 0; i < params->memory_map_size; i += params->descriptor_size) {
        struct MemoryMapEntry *entry = (struct MemoryMapEntry *)((uint8_t *)params->memory_map + i);
        switch (entry->type) {
            case KernelCode:
            case KernelData:
            case BootServicesCode:
            case BootServicesData:
            case RuntimeServicesCode:
            case RuntimeServicesData:
            case ACPIReclaimMemory:
            case ACPIMemoryNVS:
            case MemoryMappedIO:
            case MemoryMappedIOPortSpace:
            case PalCode:
            case PersistentMemory:
            case UnacceptedMemory:
                map_identity(entry);
                break;

            case ConventionalMemory:
                entry->virtual_start = ALIGN_UP_2M(entry->physical_start);
                entry->size_pages = ALIGN_DOWN_2M(entry->size_pages * PAGE_SIZE_4KB) / PAGE_SIZE_4KB;
                if (entry->virtual_start - entry->physical_start < entry->size_pages * PAGE_SIZE_4KB && entry->size_pages >= HEAP_MIN_SIZE) {
                    entry->physical_start = entry->virtual_start;
                    entry->virtual_start = next_virtual_heap_addr;
                    next_virtual_heap_addr += map_virtual(entry);
                }
                break;

            default:
                break;
        }
    }

    // Enable paging
    uint64_t pml4_phys = (uint64_t)kernel_pml4;
    uint64_t cr4_pae = (1 << 5);       // Enable PAE
    uint64_t cr0_pg  = 0x80000000;     // Enable paging

    __asm__ volatile (
        "mov %0, %%cr3\n"
        "mov %%cr4, %%rax\n"
        "or %1, %%rax\n"
        "mov %%rax, %%cr4\n"
        "mov %%cr0, %%rax\n"
        "or %2, %%rax\n"
        "mov %%rax, %%cr0\n"
        :
        : "r"(pml4_phys), "r"(cr4_pae), "r"(cr0_pg)
        : "rax"
    );

    tty_printf("[Paging] Initialized with hybrid mapping!\n");
}
