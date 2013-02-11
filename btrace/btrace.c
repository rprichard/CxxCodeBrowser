#include "btrace.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BTRACE_LOG_VAR "BTRACE_LOG"

#define EINTR_LOOP(expr)                        \
    ({                                          \
        __typeof__(expr) ret;                   \
        do {                                    \
            ret = (expr);                       \
        } while (ret == -1 && errno == EINTR);  \
        ret;                                    \
    })

typedef struct PidList PidList;
struct PidList {
    pid_t pid;
    time_t startTime;
    PidList *next;
};

static void lockFile(FILE *fp);
static void unlockFile(FILE *fp);
static char *portableGetCwd(void);
static void logExecution(FILE *logfp);
static PidList *getPidList(pid_t pid);
static PidList *reversePidList(PidList *pidList);
static void freePidList(PidList *pidList);

__attribute__((constructor))
static void processInit(void)
{
    const char *logFileName = getenv(BTRACE_LOG_VAR);
    if (logFileName == NULL || logFileName[0] == '\0')
        return;

    FILE *logfp = fopen(logFileName, "a");
    assert(logfp != NULL && "Error opening trace file for append.");
    lockFile(logfp);
    logExecution(logfp);
    fflush(logfp);
    unlockFile(logfp);
    fclose(logfp);
}

static void lockFile(FILE *fp)
{
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    int lockRet = EINTR_LOOP(fcntl(fileno(fp), F_SETLKW, &lock));
    assert(lockRet == 0 && "Error locking trace file.");
}

static void unlockFile(FILE *fp)
{
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    int unlockRet = fcntl(fileno(fp), F_SETLK, &lock);
    assert(unlockRet == 0 && "Error unlocking trace file.");
}

/* This function behaves the same as getcwd(NULL, 0) on most Unix operating
 * systems (Linux/glibc, BSD, OS X).  Unfortunately, getcwd(NULL, 0) is not
 * supported by Solaris or POSIX.1-2008. */
static char *portableGetCwd(void)
{
    char *buffer = NULL;
    for (size_t size = 1024; size >= 1024; size *= 2) {
        buffer = realloc(buffer, size);
        assert(buffer != NULL && "realloc() call failed");
        char *ret = getcwd(buffer, size);
        if (ret != NULL || errno != ERANGE)
            return ret;
    }
    return NULL;
}

static void logExecution(FILE *logfp)
{
    fputs("{", logfp);

    {
        fputs("\"cwd\":", logfp);
        char *cwd = portableGetCwd();
        btrace_writeJsonStr(logfp, cwd != NULL ? cwd : "");
        free(cwd);
        fputs(",", logfp);
    }

    {
        PidList *const pidList = reversePidList(getPidList(getpid()));
        assert(pidList != NULL && "Error getting PID ancestor list.");
        fputs("\"pidlist\":[", logfp);
        int prevPid = 0;
        long long prevStartTime = 0;
        for (PidList *p = pidList; p != NULL; p = p->next) {
            if (p != pidList)
                fputs(",", logfp);
            int deltaPid = (int)p->pid - prevPid;
            long long deltaStartTime = (long long)p->startTime - prevStartTime;
            fprintf(logfp, "%d,%lld", deltaPid, deltaStartTime);
            prevPid = p->pid;
            prevStartTime = p->startTime;
        }
        fputs("],", logfp);
        freePidList(pidList);
    }

    {
        fputs("\"argv\":[", logfp);
        char *argBlock = NULL;
        size_t argBlockSize = 0;
        btrace_getArgBlock(&argBlock, &argBlockSize);
        size_t argIndex = 0;
        while (argIndex < argBlockSize) {
            if (argIndex > 0)
                putc_unlocked(',', logfp);
            btrace_writeJsonStr(logfp, &argBlock[argIndex]);
            argIndex += strlen(&argBlock[argIndex]) + 1;
        }
        free(argBlock);
        fputs("]", logfp);
    }

    fputs("}\n", logfp);
}

/* Get the list of all ancestor processes (and a start time to guard against
 * wrap-around).  A parent process can die while we're building this list, and
 * in theory, the dead parent's PID could immediately be reused.  Guard against
 * this race condition by rechecking the parent PID. */
static PidList *getPidList(pid_t pid)
{
    if (pid == 0 || pid == 1) {
        /* There's no need to record information for these PIDs. */
        return NULL;
    }

    pid_t parentPid = 0;
    pid_t parentPidCheck = 0;
    time_t startTime = 0;
    time_t startTimeCheck = 0;
    bool status;
    PidList *parent = NULL;

    while (true) {
        status = btrace_procStat(pid, &parentPid, &startTime);
        if (!status)
            goto fail;
        parent = getPidList(parentPid);

        status = btrace_procStat(pid, &parentPidCheck, &startTimeCheck);
        if (!status)
            goto fail;
        if (parentPid == parentPidCheck && startTime == startTimeCheck)
            goto succeed;
        freePidList(parent);
        parent = NULL;
    }

succeed: ;
    PidList *ret = (PidList*)malloc(sizeof(PidList));
    ret->pid = pid;
    ret->startTime = startTime;
    ret->next = parent;
    return ret;

fail:
    freePidList(parent);
    return NULL;
}

static PidList *reversePidList(PidList *pidList)
{
    PidList *ret = NULL;
    while (pidList != NULL) {
        PidList *const next = pidList->next;
        pidList->next = ret;
        ret = pidList;
        pidList = next;
    }
    return ret;
}

static void freePidList(PidList *pidList)
{
    while (pidList != NULL) {
        PidList *const next = pidList->next;
        free(pidList);
        pidList = next;
    }
}

void btrace_writeJsonStr(FILE *logfp, const char *str)
{
    putc_unlocked('"', logfp);
    for (size_t i = 0; str[i] != '\0'; ++i)
        btrace_writeJsonStrChar(logfp, str[i]);
    putc_unlocked('"', logfp);
}

void btrace_writeJsonStrChar(FILE *logfp, char ch)
{
#define CASE(in, out)                   \
        case in:                        \
            putc_unlocked('\\', logfp); \
            putc_unlocked(out, logfp);  \
            break;

    /* A JSON string can contain any Unicode character other than '"' or '\' or
     * a control character.  Encode control characters specially.  Assume the
     * bytes [0x01...0x1F, 0x7F] correspond to codepoints
     * [U+0001...U+001F, U+007F], which is correct for UTF-8. */
    switch (ch) {
        CASE('"',  '"')
        CASE('\\', '\\')
        CASE('\b', 'b')
        CASE('\f', 'f')
        CASE('\n', 'n')
        CASE('\r', 'r')
        CASE('\t', 't')
        default: {
            const unsigned char uch = ch;
            if (uch <= 31 || uch == 0x7f) {
                putc_unlocked('\\', logfp);
                putc_unlocked('u', logfp);
                putc_unlocked('0', logfp);
                putc_unlocked('0', logfp);
                putc_unlocked("0123456789abcdef"[uch >> 4], logfp);
                putc_unlocked("0123456789abcdef"[uch & 0xf], logfp);
            } else {
                putc_unlocked(ch, logfp);
            }
        }
    }
}

/* It would be convenient if we could allocate a buffer upfront for the file,
 * but we need to read /proc/self/cmdline on Linux, and both stat and lseek
 * return a 0 size for this procfs file.
 *
 * The size of the file is written to sizeOut.  The content's buffer is
 * allocated with malloc and must be passed to free.  The content may contain
 * NUL characters.  This routine appends a NUL terminator to the file content;
 * this extra terminator is not counted as part of sizeOut. */
bool btrace_readEntireFile(
    const char *path,
    char **contentOut,
    size_t *sizeOut)
{
    *contentOut = NULL;
    if (sizeOut != NULL)
        *sizeOut = 0;

    FILE *fp = fopen(path, "r");
    if (fp == NULL)
        return false;

    const size_t chunkSize = 64 * 1024;
    char *chunk = malloc(chunkSize);
    assert(chunk != NULL && "malloc() call failed.");

    char *block = malloc(chunkSize);
    size_t blockSize = 0;
    size_t blockBufSize = chunkSize;
    assert(block != NULL && "malloc() call failed.");

    while (true) {
        size_t bytesRead = fread(chunk, 1, chunkSize, fp);
        if (bytesRead == 0)
            break;
        if (blockSize + bytesRead > blockBufSize) {
            blockBufSize = blockBufSize * 2;
            block = realloc(block, blockBufSize);
            assert(block != NULL && "realloc() call failed.");
        }
        memcpy(block + blockSize, chunk, bytesRead);
        blockSize += bytesRead;
    }

    assert(feof(fp) && !ferror(fp));
    fclose(fp);
    free(chunk);

    /* Ensure that the data is NUL-terminated. */
    block = realloc(block, blockSize + 1);
    block[blockSize] = '\0';

    *contentOut = block;
    if (sizeOut != NULL)
        *sizeOut = blockSize;

    return true;
}

void btrace_makeArgBlockWithArgcArgv(
    char **argBlock,
    size_t *argBlockSize,
    int argc,
    char **argv)
{
    size_t blockSize = 0;
    for (int i = 0; i < argc; ++i)
        blockSize += strlen(argv[i]) + 1;
    char *block = (char*)malloc(blockSize);
    assert(block != NULL && "malloc() call failed.");
    char *out = block;
    for (int i = 0; i < argc; ++i) {
        strcpy(out, argv[i]);
        out += strlen(out) + 1;
    }
    assert(out == &block[blockSize]);
    *argBlock = block;
    *argBlockSize = blockSize;
}
