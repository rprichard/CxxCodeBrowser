/*
 * Copyright 2012 Ryan Prichard.  Licensed under the MIT license.
 * See ../LICENSE.txt or http://opensource.org/licenses/MIT
 */

/*
 * ELF Version Information (EVI).
 */

#ifndef EVIREADER_H
#define EVIREADER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
    EviSize32 = 1,          /* ELFCLASS32 */
    EviSize64 = 2           /* ELFCLASS64 */
} EviSize;

typedef enum {
    EviEndianLittle = 1,    /* ELFDATA2LSB */
    EviEndianBig = 2        /* ELFDATA2MSB */
} EviEndian;

#define EviHeaderIdentSize 16
#define EviInvalidAddress ((uint64_t)~0)

typedef enum {
    EVI_PT_NULL     = 0,
    EVI_PT_LOAD     = 1,
    EVI_PT_DYNAMIC  = 2,
    EVI_PT_INTERP   = 3,
    EVI_PT_NOTE     = 4,
    EVI_PT_SHLIB    = 5,
    EVI_PT_PHDR     = 6,
    EVI_PT_TLS      = 7,
    EVI_PT_LOOS     = 0x60000000,
    EVI_PT_HIOS     = 0x6fffffff,
    EVI_PT_LOPROC   = 0x70000000,
    EVI_PT_HIPROC   = 0x7fffffff
} EviProgramType;

typedef enum {
    EVI_DT_NULL             = 0,
    EVI_DT_NEEDED           = 1,
    EVI_DT_PLTRELSZ         = 2,
    EVI_DT_PLTGOT           = 3,
    EVI_DT_HASH             = 4,
    EVI_DT_STRTAB           = 5,
    EVI_DT_SYMTAB           = 6,
    EVI_DT_RELA             = 7,
    EVI_DT_RELASZ           = 8,
    EVI_DT_RELAENT          = 9,
    EVI_DT_STRSZ            = 10,
    EVI_DT_SYMENT           = 11,
    EVI_DT_INIT             = 12,
    EVI_DT_FINI             = 13,
    EVI_DT_SONAME           = 14,
    EVI_DT_RPATH            = 15,
    EVI_DT_SYMBOLIC         = 16,
    EVI_DT_REL              = 17,
    EVI_DT_RELSZ            = 18,
    EVI_DT_RELENT           = 19,
    EVI_DT_PLTREL           = 20,
    EVI_DT_DEBUG            = 21,
    EVI_DT_TEXTREL          = 22,
    EVI_DT_JMPREL           = 23,
    EVI_DT_BIND_NOW         = 24,
    EVI_DT_INIT_ARRAY       = 25,
    EVI_DT_FINI_ARRAY       = 26,
    EVI_DT_INIT_ARRAYSZ     = 27,
    EVI_DT_FINI_ARRAYSZ     = 28,
    EVI_DT_RUNPATH          = 29,
    EVI_DT_FLAGS            = 30,
    EVI_DT_ENCODING         = 32,
    EVI_DT_PREINIT_ARRAY    = 32,
    EVI_DT_PREINIT_ARRAYSZ  = 33,
    EVI_DT_LOOS             = 0x6000000D,
    EVI_DT_HIOS             = 0x6ffff000,
    EVI_DT_LOPROC           = 0x70000000,
    EVI_DT_HIPROC           = 0x7fffffff,

    /* From /usr/include/elf.h. */
    EVI_GNU_DT_VERDEF       = 0x6ffffffc,
    EVI_GNU_DT_VERDEFNUM    = 0x6ffffffd,
    EVI_GNU_DT_VERNEED      = 0x6ffffffe,
    EVI_GNU_DT_VERNEEDNUM   = 0x6fffffff,
} EviDynamicType;

typedef struct {
    uint8_t e_ident[EviHeaderIdentSize];
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
} EviHeader;

typedef struct {
    uint32_t p_type;    /* EviProgramType */
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} EviProgramHeader;

typedef struct {
    uint64_t d_tag;     /* EviDynamicType */
    uint64_t d_val;
} EviDynamicEntry;

typedef struct {
    uint16_t vd_version;
    uint16_t vd_flags;
    uint16_t vd_ndx;
    uint16_t vd_cnt;
    uint32_t vd_hash;
    uint32_t vd_aux;
    uint32_t vd_next;
} EviGnuVerDef;

typedef struct {
    uint32_t vda_name;
    uint32_t vda_next;
} EviGnuVerDefAux;

typedef struct {
    uint16_t vn_version;
    uint16_t vn_cnt;
    uint32_t vn_file;
    uint32_t vn_aux;
    uint32_t vn_next;
} EviGnuVerNeed;

typedef struct {
    uint32_t vna_hash;
    uint16_t vna_flags;
    uint16_t vna_other;
    uint32_t vna_name;
    uint32_t vna_next;
} EviGnuVerNeedAux;

typedef struct {
    FILE *fp;
    EviSize size;
    EviEndian endian;
    EviHeader header;
    int numProgramHeaders;
    int numDynamicEntries;
    EviProgramHeader *programHeaders;
    EviDynamicEntry *dynamicEntries;
    uint64_t dynStrTab;
    uint64_t dynStrTabSize;
} Evi;

bool evi_open(Evi *evi, const char *filename);
void evi_close(Evi *evi);
char *evi_readDynStr(Evi *evi, uint64_t offset);
bool evi_readGnuVerDef(EviGnuVerDef *ret, Evi *evi, uint64_t vaddr);
bool evi_readGnuVerDefAux(EviGnuVerDefAux *ret, Evi *evi, uint64_t vaddr);
bool evi_readGnuVerNeed(EviGnuVerNeed *ret, Evi *evi, uint64_t vaddr);
bool evi_readGnuVerNeedAux(EviGnuVerNeedAux *ret, Evi *evi, uint64_t vaddr);

#endif /* EVIREADER_H */
