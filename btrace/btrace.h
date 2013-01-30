#ifndef BTRACE_H
#define BTRACE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* OS-independent. */

void btrace_writeJsonStr(FILE *logfp, const char *str);
void btrace_writeJsonStrChar(FILE *logfp, char ch);
bool btrace_readEntireFile(
    const char *path,
    char **contentOut,
    size_t *sizeOut);
void btrace_makeArgBlockWithArgcArgv(
    char **argBlock,
    size_t *argBlockSize,
    int argc,
    char **argv);

/* OS-dependent. */

/* Return a malloc-allocated block of memory containing each argument,
 * terminated with a NUL character, concatenated together.  (The buffer has
 * exactly as many NUL character as arguments.  If it is non-empty, its last
 * character is NUL.) */
void btrace_getArgBlock(char **argBlock, size_t *argBlockSize);

/* Gets the parent PID and start time of the PID.  Returns true on success and
 * false on failure. */
bool btrace_procStat(pid_t pid, pid_t *parentPid, time_t *startTime);

#endif /* BTRACE_H */
