#include "btrace.h"

/* Include this early because sys/proc.h apparently needs it for struct
 * itimerval but does not include it itself.  This problem was observed on
 * OS X 10.8. */
#include <sys/time.h>

#include <assert.h>
#include <crt_externs.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

void btrace_getArgBlock(char **argBlock, size_t *argBlockSize)
{
    btrace_makeArgBlockWithArgcArgv(
        argBlock, argBlockSize,
        *_NSGetArgc(), *_NSGetArgv());
}

bool btrace_procStat(pid_t pid, pid_t *parentPid, time_t *startTime)
{
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };
    struct kinfo_proc proc;
    size_t size = sizeof(proc);
    int ret = sysctl(mib, 4, &proc, &size, NULL, 0);
    if (ret != 0)
        return false;

    *parentPid = proc.kp_eproc.e_ppid;
    *startTime = proc.kp_proc.p_starttime.tv_sec;

    return true;
}
