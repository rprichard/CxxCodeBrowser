/*
 * Copyright 2012 Ryan Prichard.  Licensed under the MIT license.
 * See ../LICENSE.txt or http://opensource.org/licenses/MIT
 */

/*
 * ELF Version Information (EVI).
 *
 * References:
 *  - Generic ABI (gABI) / SysV ABI.
 *    http://www.sco.com/developers/gabi/latest/contents.html
 *  - Symbol Versioning.  Linux Standard Base Core Specification 3.2.
 *    http://refspecs.linuxfoundation.org/LSB_3.2.0/LSB-Core-generic/LSB-Core-generic/symversion.html
 */

#include "evi_reader.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool evi_read(void *buf, const Evi *evi, size_t amount)
{
    size_t result = fread(buf, 1, amount, evi->fp);
    if (result != amount)
        return false;
    return true;
}

static bool evi_readUint64(uint64_t *ret, const Evi *evi)
{
    *ret = 0;
    uint8_t data[8];
    if (!evi_read(data, evi, sizeof(data)))
        return false;
    const uint64_t d0 = data[0];
    const uint64_t d1 = data[1];
    const uint64_t d2 = data[2];
    const uint64_t d3 = data[3];
    const uint64_t d4 = data[4];
    const uint64_t d5 = data[5];
    const uint64_t d6 = data[6];
    const uint64_t d7 = data[7];
    if (evi->endian == EviEndianLittle) {
        *ret = (d0 <<  0) | (d1 <<  8) | (d2 << 16) | (d3 << 24) |
               (d4 << 32) | (d5 << 40) | (d6 << 48) | (d7 << 56);
    } else {
        *ret = (d7 <<  0) | (d6 <<  8) | (d5 << 16) | (d4 << 24) |
               (d3 << 32) | (d2 << 40) | (d1 << 48) | (d0 << 56);
    }
    return true;
}

static bool evi_readUint32(uint32_t *ret, const Evi *evi)
{
    *ret = 0;
    uint8_t data[4];
    if (!evi_read(data, evi, sizeof(data)))
        return false;
    const uint32_t d0 = data[0];
    const uint32_t d1 = data[1];
    const uint32_t d2 = data[2];
    const uint32_t d3 = data[3];
    if (evi->endian == EviEndianLittle)
        *ret = d0 | (d1 << 8) | (d2 << 16) | (d3 << 24);
    else
        *ret = d3 | (d2 << 8) | (d1 << 16) | (d0 << 24);
    return true;
}

static bool evi_readUint16(uint16_t *ret, const Evi *evi)
{
    *ret = 0;
    uint8_t data[2];
    if (!evi_read(data, evi, sizeof(data)))
        return false;
    const uint16_t d0 = data[0];
    const uint16_t d1 = data[1];
    if (evi->endian == EviEndianLittle)
        *ret = d0 | (d1 << 8);
    else
        *ret = d1 | (d0 << 8);
    return true;
}

static __attribute__((unused)) bool evi_readUint8(
    uint8_t *ret, const Evi *evi)
{
    *ret = 0;
    return evi_read(ret, evi, 1);
}

static bool evi_readAddr(uint64_t *ret, const Evi *evi)
{
    if (evi->size == EviSize32) {
        uint32_t temp = 0;
        if (!evi_readUint32(&temp, evi))
            return false;
        *ret = temp;
        return true;
    } else {
        return evi_readUint64(ret, evi);
    }
}

static bool evi_readProgramHeader(EviProgramHeader *ret, const Evi *evi)
{
    memset(ret, 0, sizeof(EviProgramHeader));
    long startOffset = ftell(evi->fp);
    if (!evi_readUint32(&ret->p_type, evi)) goto error;
    if (evi->size == EviSize64) {
        if (!evi_readUint32(&ret->p_flags, evi)) goto error;
    }
    if (!evi_readAddr(&ret->p_offset, evi)) goto error;
    if (!evi_readAddr(&ret->p_vaddr, evi)) goto error;
    if (!evi_readAddr(&ret->p_paddr, evi)) goto error;
    if (!evi_readAddr(&ret->p_filesz, evi)) goto error;
    if (!evi_readAddr(&ret->p_memsz, evi)) goto error;
    if (evi->size == EviSize32) {
        if (!evi_readUint32(&ret->p_flags, evi)) goto error;
    }
    if (!evi_readAddr(&ret->p_align, evi)) goto error;
    long amountRead = ftell(evi->fp) - startOffset;
    if (amountRead != evi->header.e_phentsize) {
        /* Neither glibc nor BSD's libelf accommodate an e_phentsize other than
         * the expected one. */
        goto error;
    }
    return true;
error:
    return false;
}

static bool evi_initDynamic(Evi *evi)
{
    int dynamicIndex = -1;
    for (int pi = 0; pi < evi->numProgramHeaders; ++pi) {
        if (evi->programHeaders[pi].p_type != EVI_PT_DYNAMIC)
            continue;
        if (dynamicIndex != -1)
            goto error;
        dynamicIndex = pi;
    }
    if (dynamicIndex != -1) {
        EviProgramHeader *ph = &evi->programHeaders[dynamicIndex];
        if (ph->p_memsz != ph->p_filesz) goto error;
        int entrySize = evi->size == EviSize32 ? 8 : 16;
        int entryCount = ph->p_memsz / entrySize;
        evi->numDynamicEntries = entryCount;
        evi->dynamicEntries = (EviDynamicEntry*)malloc(
            sizeof(EviDynamicEntry) * entryCount);
        if (evi->dynamicEntries == NULL) goto error;
        if (fseek(evi->fp, ph->p_offset, SEEK_SET) != 0) goto error;
        for (int di = 0; di < entryCount; ++di) {
            EviDynamicEntry *de = &evi->dynamicEntries[di];
            if (!evi_readAddr(&de->d_tag, evi)) goto error;
            if (!evi_readAddr(&de->d_val, evi)) goto error;
            if (de->d_tag == EVI_DT_NULL) {
                /* According to the ELF specification, a DT_NULL entry marks
                 * the end of the _DYNAMIC array, and therefore the end of the
                 * dynamic entries. */
                evi->numDynamicEntries = di;
                break;
            }
        }
    }
    bool foundStrTab = false;
    bool foundStrTabSize = false;
    for (int di = 0; di < evi->numDynamicEntries; ++di) {
        EviDynamicEntry *de = &evi->dynamicEntries[di];
        if (de->d_tag == EVI_DT_STRTAB) {
            if (foundStrTab) goto error;
            foundStrTab = true;
            evi->dynStrTab = de->d_val;
        }
        if (de->d_tag == EVI_DT_STRSZ) {
            if (foundStrTabSize) goto error;
            foundStrTabSize = true;
            evi->dynStrTabSize = de->d_val;
        }
    }
    if (!foundStrTab || !foundStrTabSize) {
        evi->dynStrTab = EviInvalidAddress;
        evi->dynStrTabSize = 0;
    }
    return true;
error:
    return false;
}

static uint64_t evi_translateVAddrToOffset(Evi *evi, uint64_t vaddr)
{
    for (int pi = 0; pi < evi->numProgramHeaders; ++pi) {
        EviProgramHeader *ph = &evi->programHeaders[pi];
        if (vaddr >= ph->p_vaddr &&
            vaddr < ph->p_vaddr + ph->p_filesz)
            return ph->p_offset + (vaddr - ph->p_vaddr);
    }
    return EviInvalidAddress;
}

static bool evi_seekToVAddr(Evi *evi, uint64_t vaddr)
{
    uint64_t offset = evi_translateVAddrToOffset(evi, vaddr);
    if (offset == EviInvalidAddress)
        return false;
    if (fseek(evi->fp, offset, SEEK_SET) != 0)
        return false;
    return true;
}

bool evi_open(Evi *evi, const char *filename)
{
    memset(evi, 0, sizeof(Evi));
    evi->fp = fopen(filename, "rb");
    if (evi->fp == NULL)
        goto error;

    /* Read the ELF header. */
    if (!evi_read(&evi->header.e_ident, evi, EviHeaderIdentSize))
        goto error;
    if (memcmp(evi->header.e_ident, "\x7F""ELF", 4) != 0)
        goto error;
    evi->size = evi->header.e_ident[4];
    evi->endian = evi->header.e_ident[5];
    if (!evi_readUint16(&evi->header.e_type, evi)) goto error;
    if (!evi_readUint16(&evi->header.e_machine, evi)) goto error;
    if (!evi_readUint32(&evi->header.e_version, evi)) goto error;
    if (!evi_readAddr(&evi->header.e_entry, evi)) goto error;
    if (!evi_readAddr(&evi->header.e_phoff, evi)) goto error;
    if (!evi_readAddr(&evi->header.e_shoff, evi)) goto error;
    if (!evi_readUint32(&evi->header.e_flags, evi)) goto error;
    if (!evi_readUint16(&evi->header.e_ehsize, evi)) goto error;
    if (!evi_readUint16(&evi->header.e_phentsize, evi)) goto error;
    if (!evi_readUint16(&evi->header.e_phnum, evi)) goto error;
    if (!evi_readUint16(&evi->header.e_shentsize, evi)) goto error;
    if (!evi_readUint16(&evi->header.e_shnum, evi)) goto error;
    if (!evi_readUint16(&evi->header.e_shstrndx, evi)) goto error;

    /* Read the program headers. */
    evi->numProgramHeaders = evi->header.e_phnum;
    evi->programHeaders = (EviProgramHeader*)malloc(
        sizeof(EviProgramHeader) * evi->numProgramHeaders);
    if (evi->programHeaders == NULL) goto error;
    for (int pi = 0; pi < evi->numProgramHeaders; ++pi)
        evi_readProgramHeader(&evi->programHeaders[pi], evi);

    /* Read the EVI_PT_DYNAMIC program header, if it exists. */
    if (!evi_initDynamic(evi))
        goto error;

    return true;

error:
    if (evi->fp != NULL)
        fclose(evi->fp);
    free(evi->programHeaders);
    free(evi->dynamicEntries);
    return false;
}

void evi_close(Evi *evi)
{
    if (evi->fp != NULL)
        fclose(evi->fp);
    free(evi->programHeaders);
    free(evi->dynamicEntries);
    evi->fp = NULL;
    evi->programHeaders = NULL;
    evi->dynamicEntries = NULL;
}

char *evi_readDynStr(Evi *evi, uint64_t offset)
{
    if (evi->dynStrTab == EviInvalidAddress || offset >= evi->dynStrTabSize)
        return NULL;
    int retBufSize = 0;
    int retSize = 0;
    char *ret = NULL;
    if (!evi_seekToVAddr(evi, evi->dynStrTab + offset))
        goto error;
    int ch;
    do {
        ch = fgetc(evi->fp);
        if (ch < 0)
            goto error;
        if (retSize == retBufSize) {
            retBufSize *= 2;
            if (retBufSize < 128)
                retBufSize = 128;
            ret = realloc(ret, retBufSize);
            if (ret == NULL)
                goto error;
        }
        ret[retSize++] = ch;
    } while (ch != '\0');
    assert(ret != NULL);
    return ret;
error:
    free(ret);
    return NULL;
}

bool evi_readGnuVerDef(EviGnuVerDef *ret, Evi *evi, uint64_t vaddr)
{
    if (!evi_seekToVAddr(evi, vaddr)) goto error;
    if (!evi_readUint16(&ret->vd_version, evi)) goto error;
    if (!evi_readUint16(&ret->vd_flags, evi)) goto error;
    if (!evi_readUint16(&ret->vd_ndx, evi)) goto error;
    if (!evi_readUint16(&ret->vd_cnt, evi)) goto error;
    if (!evi_readUint32(&ret->vd_hash, evi)) goto error;
    if (!evi_readUint32(&ret->vd_aux, evi)) goto error;
    if (!evi_readUint32(&ret->vd_next, evi)) goto error;
    return true;
error:
    return false;
}

bool evi_readGnuVerDefAux(EviGnuVerDefAux *ret, Evi *evi, uint64_t vaddr)
{
    if (!evi_seekToVAddr(evi, vaddr)) goto error;
    if (!evi_readUint32(&ret->vda_name, evi)) goto error;
    if (!evi_readUint32(&ret->vda_next, evi)) goto error;
    return true;
error:
    return false;
}

bool evi_readGnuVerNeed(EviGnuVerNeed *ret, Evi *evi, uint64_t vaddr)
{
    if (!evi_seekToVAddr(evi, vaddr)) goto error;
    if (!evi_readUint16(&ret->vn_version, evi)) goto error;
    if (!evi_readUint16(&ret->vn_cnt, evi)) goto error;
    if (!evi_readUint32(&ret->vn_file, evi)) goto error;
    if (!evi_readUint32(&ret->vn_aux, evi)) goto error;
    if (!evi_readUint32(&ret->vn_next, evi)) goto error;
    return true;
error:
    return false;
}

bool evi_readGnuVerNeedAux(EviGnuVerNeedAux *ret, Evi *evi, uint64_t vaddr)
{
    if (!evi_seekToVAddr(evi, vaddr)) goto error;
    if (!evi_readUint32(&ret->vna_hash, evi)) goto error;
    if (!evi_readUint16(&ret->vna_flags, evi)) goto error;
    if (!evi_readUint16(&ret->vna_other, evi)) goto error;
    if (!evi_readUint32(&ret->vna_name, evi)) goto error;
    if (!evi_readUint32(&ret->vna_next, evi)) goto error;
    return true;
error:
    return false;
}
