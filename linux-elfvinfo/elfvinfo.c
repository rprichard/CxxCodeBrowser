/*
 * Copyright 2012 Ryan Prichard.  Licensed under the MIT license.
 * See ../LICENSE.txt or http://opensource.org/licenses/MIT
 */

/*
 * A utility that assists with compatibility checking of ELF shared objects.
 *  - It can dump the DT_NEEDED / DT_VERNEED / DT_VERDEF identifiers in an
 *    ELF file.
 *  - It can find a shared object using dlopen, or it can open any ELF file
 *    with a path.
 *
 * Compile with:
 * gcc elfvinfo.c evi_reader.c -o elfvinfo -std=c99 -ldl
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <dlfcn.h>
#include <link.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "evi_reader.h"

typedef struct {
    bool dlopenSearch;
    bool dlopenPath;
    bool showNeeded;
    bool showVerDef;
    bool showVerNeed;
    char *elfname;
} Options;

static void parseCommandLine(Options *options, int argc, char *argv[]);
static void usage(const char *argv0);
static void showNeeded(Evi *evi);
static void showVerDef(Evi *evi);
static void showVerNeed(Evi *evi);

int main(int argc, char *argv[])
{
    Options options;
    parseCommandLine(&options, argc, argv);

    if (options.dlopenSearch) {
        void *lib = dlopen(options.elfname, RTLD_LAZY);
        if (lib == NULL) {
            fprintf(stderr, "error: dlopen failed on: %s\n", options.elfname);
            exit(1);
        }
        struct link_map *map = NULL;
        if (dlinfo(lib, RTLD_DI_LINKMAP, &map) != 0) {
            fprintf(stderr, "error: dlinfo failed: %s\n", dlerror());
            exit(1);
        }
        free(options.elfname);
        options.elfname = strdup(map->l_name);
        dlclose(lib);
        if (options.dlopenPath)
            printf("%s\n", options.elfname);
    }

    Evi evi;
    if (!evi_open(&evi, options.elfname)) {
        fprintf(stderr, "error: could not read ELF file: %s\n",
            options.elfname);
        exit(1);
    }

    if (options.showNeeded)
        showNeeded(&evi);
    if (options.showVerDef)
        showVerDef(&evi);
    if (options.showVerNeed)
        showVerNeed(&evi);

    free(options.elfname);
    options.elfname = NULL;
    evi_close(&evi);

    return 0;
}

static void parseCommandLine(Options *options, int argc, char *argv[])
{
    memset(options, 0, sizeof(Options));
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--dlopen")) {
            options->dlopenSearch = true;
        } else if (!strcmp(argv[i], "--dlopen-path")) {
            options->dlopenPath = true;
        } else if (!strcmp(argv[i], "--needed")) {
            options->showNeeded = true;
        } else if (!strcmp(argv[i], "--verdef")) {
            options->showVerDef = true;
        } else if (!strcmp(argv[i], "--verneed")) {
            options->showVerNeed = true;
        } else if (!strcmp(argv[i], "--help")) {
            usage(argv[0]);
            exit(0);
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "error: Unrecognized option: %s\n", argv[i]);
            exit(1);
        } else {
            if (options->elfname != NULL) {
                fprintf(stderr, "error: Multiple files specified.\n");
                exit(1);
            }
            options->elfname = strdup(argv[i]);
        }
    }
    if (options->elfname == NULL) {
        fprintf(stderr, "error: No file name specified.\n");
        usage(argv[0]);
        exit(1);
    }
}

static void usage(const char *argv0)
{
    printf("Usage: %s [options] elfname\n", argv0);
    printf("Prints version information for an ELF file.\n");
    printf("Options:\n");
    printf("   --dlopen    Find elfname using dlopen.  It may be an soname.\n");
    printf("   --dlopen-path\n");
    printf("               If --dlopen was also specified, print the absolute path\n");
    printf("               to the opened library.\n");
    printf("   --needed    Print each DT_NEEDED path.\n");
    printf("   --verdef    Print each version defined in the DT_VERDEF section.\n");
    printf("   --verneed   Print each file/version pair in the DT_VERNEED section.\n");
}

static void showNeeded(Evi *evi)
{
    for (int di = 0; di < evi->numDynamicEntries; ++di) {
        EviDynamicEntry *de = &evi->dynamicEntries[di];
        if (de->d_tag == EVI_DT_NEEDED) {
            char *file = evi_readDynStr(evi, de->d_val);
            if (file == NULL) {
                fprintf(stderr, "error: Could not read NEEDED string\n");
                exit(1);
            }
            printf("%s\n", file);
            free(file);
        }
    }
}

static void showVerDef(Evi *evi)
{
    uint64_t verDefTable = EviInvalidAddress;
    int verDefTableCount = 0;
    for (int di = 0; di < evi->numDynamicEntries; ++di) {
        EviDynamicEntry *de = &evi->dynamicEntries[di];
        if (de->d_tag == EVI_GNU_DT_VERDEF) {
            verDefTable = de->d_val;
        } else if (de->d_tag == EVI_GNU_DT_VERDEFNUM) {
            verDefTableCount = de->d_val;
        }
    }
    if (verDefTable == EviInvalidAddress || verDefTableCount == 0)
        return;
    uint64_t verIter = verDefTable;
    EviGnuVerDef ver;
    EviGnuVerDefAux aux;
    for (int i = 0; i < verDefTableCount; ++i, verIter += ver.vd_next) {
        if (!evi_readGnuVerDef(&ver, evi, verIter)) {
            fprintf(stderr, "error: Could not read VERDEF #%d\n", i);
            exit(1);
        }
        if (ver.vd_version != 1) {
            fprintf(stderr, "error: Found vd_version %d; expected 1\n",
                ver.vd_version);
            exit(1);
        }
        if (ver.vd_cnt == 0)
            continue;
        if (!evi_readGnuVerDefAux(&aux, evi, verIter + ver.vd_aux)) {
            fprintf(stderr, "error: Could not read VERDEF #%d aux\n", i);
            exit(1);
        }
        char *name = evi_readDynStr(evi, aux.vda_name);
        if (name == NULL) {
            fprintf(stderr, "error: Could not read VERDEF #%d name\n", i);
            exit(1);
        }
        printf("%s\n", name);
        free(name);
    }
}

static void showVerNeed(Evi *evi)
{
    uint64_t verNeedTable = EviInvalidAddress;
    int verNeedTableCount = 0;
    for (int di = 0; di < evi->numDynamicEntries; ++di) {
        EviDynamicEntry *de = &evi->dynamicEntries[di];
        if (de->d_tag == EVI_GNU_DT_VERNEED) {
            verNeedTable = de->d_val;
        } else if (de->d_tag == EVI_GNU_DT_VERNEEDNUM) {
            verNeedTableCount = de->d_val;
        }
    }
    uint64_t verIter = verNeedTable;
    EviGnuVerNeed ver;
    EviGnuVerNeedAux aux;
    for (int vi = 0; vi < verNeedTableCount; ++vi, verIter += ver.vn_next) {
        if (!evi_readGnuVerNeed(&ver, evi, verIter)) {
            fprintf(stderr, "error: Could not read VERNEED #%d\n", vi);
            exit(1);
        }
        if (ver.vn_version != 1) {
            fprintf(stderr, "error: Found vn_version %d; expected 1\n",
                ver.vn_version);
            exit(1);
        }
        if (ver.vn_cnt == 0)
            continue;
        char *file = evi_readDynStr(evi, ver.vn_file);
        if (file == NULL) {
            fprintf(stderr,
                "error: Could not read VERNEED #%d file\n", vi);
            exit(1);
        }
        uint64_t auxIter = verIter + ver.vn_aux;
        for (int ai = 0; ai < ver.vn_cnt; ++ai, auxIter += aux.vna_next) {
            if (!evi_readGnuVerNeedAux(&aux, evi, auxIter)) {
                fprintf(stderr, "error: Could not read VERNEED #%d aux #%d\n",
                    vi, ai);
                exit(1);
            }
            char *verName = evi_readDynStr(evi, aux.vna_name);
            if (verName == NULL) {
                fprintf(stderr, "error: Could not read VERNEED #%d name #%d\n",
                    vi, ai);
                exit(1);
            }
            printf("%s\t%s\n", file, verName);
            free(verName);
        }
        free(file);
    }
}
