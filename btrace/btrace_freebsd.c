#include "btrace.h"

#include <assert.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <paths.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>
#include <time.h>
#include <unistd.h>

void btrace_getArgBlock(char **argBlock, size_t *argBlockSize)
{
    char errbuf[_POSIX2_LINE_MAX];
    errbuf[0] = '\0';
    kvm_t *kvm = kvm_openfiles(NULL, _PATH_DEVNULL, NULL, O_RDONLY, errbuf);
    if (kvm == NULL) {
        fprintf(stderr, "btrace error: kvm_openfiles failed: %s\n", errbuf);
        abort();
    }
    int cnt = 0;
    struct kinfo_proc *proc = kvm_getprocs(kvm, KERN_PROC_PID, getpid(), &cnt);
    assert(proc != NULL && cnt == 1);
    char **argv = kvm_getargv(kvm, proc, 0);
    assert(argv != NULL);
    int argc = 0;
    while (argv[argc] != NULL)
        ++argc;
    btrace_makeArgBlockWithArgcArgv(argBlock, argBlockSize, argc, argv);
    kvm_close(kvm);
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
