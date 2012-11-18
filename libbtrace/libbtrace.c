#include <alloca.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

/* The code in this file must call only async-signal-safe POSIX/C functions. */

#define BTRACE_LOG_ENV_VAR "BTRACE_LOG"

static void safeAssertFail(
        const char *filename,
        const char *line,
        const char *func,
        const char *condition)
{
    const char *parts[] = {
        "libbtrace.so: ", filename, ":", line, ": ",
        func, ": Assertion `", condition, "' failed.\n", NULL
    };
    size_t total = 0;
    for (int i = 0; parts[i] != NULL; ++i)
        total += strlen(parts[i]);
    char *message = alloca(total + 1);
    message[0] = '\0';
    for (int i = 0; parts[i] != NULL; ++i)
        strcat(message, parts[i]);
    write(STDERR_FILENO, message, total);
    abort();
}

#define STRINGIFY_HELPER(x) #x
#define STRINGIFY(x) STRINGIFY_HELPER(x)
#define safeAssert(condition) \
        if (!(condition)) \
            safeAssertFail(__FILE__, STRINGIFY(__LINE__), \
                           __FUNCTION__, #condition)

#define EINTR_LOOP(expr)                        \
    ({                                          \
        __typeof__(expr) ret;                   \
        do {                                    \
            ret = (expr);                       \
        } while (ret == -1 && errno == EINTR);  \
        ret;                                    \
    })

static int (*real_execvpe)(const char*, char *const[], char *const[]) = NULL;
static int (*real_execve)(const char*, char *const[], char *const[]) = NULL;
static char g_logFileName[1024];

void _init(void)
{
    /* dlsym is not async-signal-safe, so try calling it up-front in _init
     * rather than lazily. */
    real_execvpe = dlsym(RTLD_NEXT, "execvpe");
    real_execve = dlsym(RTLD_NEXT, "execve");
    const char *logFileVar = getenv(BTRACE_LOG_ENV_VAR);
    if (logFileVar != NULL && strlen(logFileVar) < sizeof(g_logFileName))
        strcpy(g_logFileName, logFileVar);
}

typedef struct {
    int fd;
    char buf[1024];
    int bufCount;
} LogFile;

static void openLogFile(LogFile *logFile, const char *filename)
{
    logFile->fd = 0;
    logFile->bufCount = 0;
    errno = 0;
    logFile->fd = EINTR_LOOP(open(
            filename, O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0644));
    safeAssert(logFile->fd != -1 && "Error opening trace file for append.");
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    int lockRet = EINTR_LOOP(fcntl(logFile->fd, F_SETLKW, &lock));
    safeAssert(lockRet == 0 && "Error locking trace file.");
}

static void flushLogFile(LogFile *logFile)
{
    ssize_t amount = EINTR_LOOP(
                write(logFile->fd, logFile->buf, logFile->bufCount));
    safeAssert(amount == logFile->bufCount && "Error writing to trace file.");
    logFile->bufCount = 0;
}

static inline void writeChar(LogFile *logFile, char ch)
{
    if (logFile->bufCount == sizeof(logFile->buf))
        flushLogFile(logFile);
    logFile->buf[logFile->bufCount++] = ch;
}

static void closeLogFile(LogFile *logFile)
{
    flushLogFile(logFile);
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    int unlockRet = fcntl(logFile->fd, F_SETLK, &lock);
    safeAssert(unlockRet == 0 && "Error unlocking trace file.");
    close(logFile->fd);
}

/* Write the string to the LogFile.  Spaces and newlines are quoted.
 * Backslashes and quotes are escaped. */
static void writeEscapedString(LogFile *logFile, const char *text)
{
    bool needsQuotes =
        (strchr(text, ' ') != NULL ||
         strchr(text, '\n') != NULL);
    if (needsQuotes)
        writeChar(logFile, '\"');
    for (int i = 0; text[i] != '\0'; ++i) {
        char ch = text[i];
        if (ch == '\\' || ch == '\"')
            writeChar(logFile, '\\');
        writeChar(logFile, ch);
    }
    if (needsQuotes)
        writeChar(logFile, '\"');
}

static bool attemptWriteCwd(LogFile *logFile, size_t size)
{
    char *buf = alloca(size);
    ssize_t amount = readlink("/proc/self/cwd", buf, size);
    safeAssert(amount != -1 && "Error calling readlink on /proc/self/cwd");
    if ((size_t)amount < size) {
        buf[amount] = '\0';
        writeEscapedString(logFile, buf);
        writeChar(logFile, '\n');
        return true;
    } else {
        safeAssert((size_t)amount == size &&
                   "Invalid return value from readlink");
        return false;
    }
}

static void logExecution(const char *filename, char *const argv[])
{
    if (g_logFileName[0] == '\0')
        return;
    LogFile logFile;
    openLogFile(&logFile, g_logFileName);
    {
        bool success = false;
        size_t bufSize = 256;
        while (bufSize < 1024 * 1024) {
            if (attemptWriteCwd(&logFile, bufSize)) {
                success = true;
                break;
            }
            bufSize <<= 1;
        }
        safeAssert(success && "Error getting current working directory.");
    }
    {
        writeEscapedString(&logFile, filename);
        writeChar(&logFile, '\n');
        for (int i = 0; argv[i] != NULL; ++i) {
            if (i > 0)
                writeChar(&logFile, ' ');
            writeEscapedString(&logFile, argv[i]);
        }
        writeChar(&logFile, '\n');
    }
    closeLogFile(&logFile);
}

int execvpe(const char *filename,
            char *const argv[],
            char *const envp[])
{
    logExecution(filename, argv);
    return real_execvpe(filename, argv, envp);
}

int execve(const char *filename,
           char *const argv[],
           char *const envp[])
{
    logExecution(filename, argv);

    return real_execve(filename, argv, envp);
}

int execv(const char *path, char *const argv[])
{
    return execve(path, argv, environ);
}

int execvp(const char *file, char *const argv[])
{
    return execvpe(file, argv, environ);
}

int execl(const char *path, const char *arg, ...)
{
    va_list ap;

    va_start(ap, arg);
    int count = 0;
    while (va_arg(ap, const char*) != NULL)
        count++;
    va_end(ap);

    char **argv = alloca(sizeof(char*) * (count + 1));
    safeAssert(argv);
    va_start(ap, arg);
    for (int i = 0; i < count; ++i)
        argv[i] = va_arg(ap, char*);
    argv[count] = NULL;
    va_end(ap);

    return execv(path, argv);
}

int execlp(const char *file, const char *arg, ...)
{
    va_list ap;

    va_start(ap, arg);
    int count = 0;
    while (va_arg(ap, const char*) != NULL)
        count++;
    va_end(ap);

    char **argv = alloca(sizeof(char*) * (count + 1));
    safeAssert(argv);
    va_start(ap, arg);
    for (int i = 0; i < count; ++i)
        argv[i] = va_arg(ap, char*);
    argv[count] = NULL;
    va_end(ap);

    return execvp(file, argv);
}

int execle(const char *path, const char *arg, ...)
{
    va_list ap;

    va_start(ap, arg);
    int count = 0;
    while (va_arg(ap, const char*) != NULL)
        count++;
    va_end(ap);

    char **argv = alloca(sizeof(char*) * (count + 1));
    safeAssert(argv);
    va_start(ap, arg);
    for (int i = 0; i < count; ++i)
        argv[i] = va_arg(ap, char*);
    argv[count] = NULL;
    char **envp = va_arg(ap, char**);
    va_end(ap);

    return execve(path, argv, envp);
}
