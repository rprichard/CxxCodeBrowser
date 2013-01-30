#include "btrace.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>
#include <time.h>
#include <unistd.h>

void btrace_getArgBlock(char **argBlock, size_t *argBlockSize)
{
    int ret = 0;
    const int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ARGS, getpid() };
    size_t blockSize = 0;
    ret = sysctl(mib, 4, NULL, &blockSize, NULL, 0);
    assert(ret == 0 && "sysctl() KERN_PROC_ARGS call 1 failed.");
    char *block = (char*)malloc(blockSize + 1);
    assert(block != NULL && "malloc() cmdline call failed.");
    ret = sysctl(mib, 4, block, &blockSize, NULL, 0);
    assert(ret == 0 && "sysctl() KERN_PROC_ARGS call 2 failed.");
    if (blockSize > 0 && block[blockSize - 1] != '\0') {
        /* Ensure that the last argument is NUL-terminated. */
        blockSize++;
        block[blockSize - 1] = '\0';
    }
    *argBlock = block;
    *argBlockSize = blockSize;
}

bool btrace_procStat(pid_t pid, pid_t *parentPid, time_t *startTime)
{
    const int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
    struct kinfo_proc proc;
    size_t size = sizeof(proc);
    int ret = sysctl(mib, 4, &proc, &size, NULL, 0);
    if (ret != 0)
        return false;
    *parentPid = proc.ki_ppid;
    *startTime = proc.ki_start.tv_sec;
    return true;
}
