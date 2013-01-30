#include "btrace.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void btrace_getArgBlock(char **argBlock, size_t *argBlockSize)
{
    char *buf = NULL;
    size_t bufSize = 0;
    bool ret = btrace_readEntireFile("/proc/self/cmdline", &buf, &bufSize);
    assert(ret && "Error reading /proc/self/cmdline.");
    if (bufSize > 0 && buf[bufSize - 1] != '\0') {
        /* Ensure that the last argument is NUL-terminated. */
        /* [rprichard] I have observed a non-NUL-terminated cmdline with PID 1
         * on Linux Mint 13.  The file content was "/sbin/init". */
        bufSize++;
    }
    *argBlock = buf;
    *argBlockSize = bufSize;
}

static void advanceField(char **p, int i, int j)
{
    for (; i < j; ++i) {
        *p = strchr(*p, ' ');
        assert(*p != NULL && "Error reading /proc/<pid>/stat");
        (*p)++;
    }
}

bool btrace_procStat(pid_t pid, pid_t *parentPid, time_t *startTime)
{
    *parentPid = 0;
    *startTime = 0;
    bool success = false;

    int parentPidAsInt = 0;
    unsigned long long startTimeAsJiffies = 0;
    char *statContent = NULL;

    {
        char path[64];
        sprintf(path, "/proc/%d/stat", pid);
        if (!btrace_readEntireFile(path, &statContent, NULL))
            goto done;
    }

    /* Find the end of field 2 by searching for the last ')' character.
     * This is the same approach that the Linux ps utility uses.  It works
     * because no later field can contain a ')' character. */
    char *p = strrchr(statContent, ')');
    p += 2; /* Skip ") ". */

    /* p now points at field 3.  Read fields 4 (ppid) and 22
     * (starttime). */
    advanceField(&p, 3, 4);
    sscanf(p, "%d", &parentPidAsInt);
    advanceField(&p, 4, 22);
    sscanf(p, "%llu", &startTimeAsJiffies);

    /* TODO: Fix up the time units... */

    success = true;
    *parentPid = parentPidAsInt;
    *startTime = startTimeAsJiffies;

done:
    free(statContent);
    return success;
}
