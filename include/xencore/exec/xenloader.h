#ifndef _XENLOADER_H
#define _XENLOADER_H

#include <stdint.h>
#include <stdbool.h>

#define PT_LOAD 1

typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) Elf64_Phdr;

typedef struct {
    uint32_t s_name;
    uint32_t s_type;
    uint64_t s_flags;
    uint64_t s_addr;
    uint64_t s_offset;
    uint64_t s_size;
    uint32_t s_link;
    uint32_t s_info;
    uint64_t s_addralign;
    uint64_t s_entsize;
} __attribute__((packed)) Elf64_Shdr;

typedef struct {
    Elf64_Ehdr header;
    Elf64_Phdr *segments;
    Elf64_Shdr *sections;
} __attribute__((packed)) Elf64;

Elf64 *load_elf64(void* elf_data);
void free_elf64(Elf64 *elf);

#endif
