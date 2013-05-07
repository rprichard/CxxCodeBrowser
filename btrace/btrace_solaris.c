#include "btrace.h"

#include <assert.h>
#include <fcntl.h>
#include <procfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void btrace_getArgBlock(char **argBlock, size_t *argBlockSize)
{
    psinfo_t psinfo;

    int fd = open ("/proc/self/psinfo", O_RDONLY);
    assert(fd>=0 && "Error reading /proc/self/psinfo.");

    read (fd, &psinfo, sizeof(psinfo));

    int argc = psinfo.pr_argc;

    char **argv = (char **)psinfo.pr_argv;

    btrace_makeArgBlockWithArgcArgv(argBlock, argBlockSize, argc, argv);
}

bool btrace_procStat(pid_t pid, pid_t *parentPid, time_t *startTime)
{
    psinfo_t psinfo;
    char fileToOpen[128] = {0};

    snprintf(fileToOpen,  sizeof(fileToOpen) - 1, "/proc/%d/psinfo", (int)pid);

    int fd = open (fileToOpen, O_RDONLY);
    assert(fd>=0 && "Error reading /proc/<pid>/psinfo.");

    if (read (fd, &psinfo, sizeof(psinfo)) != sizeof(psinfo))
        return false;

    *parentPid = psinfo.pr_ppid;
    *startTime = psinfo.pr_start.tv_sec;

    return true;
}
