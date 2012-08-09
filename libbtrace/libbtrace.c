#include <assert.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

/* Write the string to the FILE.  Spaces and newlines are quoted.  Backslashes
 * and quotes are escaped. */
static void writeString(FILE *fp, const char *text)
{
    bool needsQuotes =
        (strchr(text, ' ') != NULL ||
         strchr(text, '\n') != NULL);
    if (needsQuotes)
        fputc('\"', fp);
    for (int i = 0; text[i] != '\0'; ++i) {
        char ch = text[i];
        if (ch == '\\' || ch == '\"')
            fputc('\\', fp);
        fputc(ch, fp);
    }
    if (needsQuotes)
        fputc('\"', fp);
}

static void logExecution(const char *filename, char *const argv[])
{
    char *logFile = getenv("BTRACE_LOG");
    if (logFile != NULL && logFile[0] != '\0') {
        FILE *fp = fopen(logFile, "a");
        flock(fileno(fp), LOCK_EX);
        {
            char path[8192];  /* TODO: replace fixed-size buffer */
            char *result = getcwd(path, sizeof(path));
            writeString(fp, result);
            fputc('\n', fp);
        }
        {
            writeString(fp, filename);
            fputc('\n', fp);
            for (int i = 0; argv[i] != NULL; ++i) {
                if (i > 0)
                    fputc(' ', fp);
                writeString(fp, argv[i]);
            }
            fputc('\n', fp);
        }
        flock(fileno(fp), LOCK_UN);
        fclose(fp);
    }
}

int execvpe(const char *filename,
            char *const argv[],
            char *const envp[])
{
    static int (*real_execvpe)(const char*, char *const[], char *const[]) = NULL;
    if (real_execvpe == NULL)
        real_execvpe = dlsym(RTLD_NEXT, "execvpe");

    logExecution(filename, argv);

    return real_execvpe(filename, argv, envp);
}

int execve(const char *filename,
           char *const argv[],
           char *const envp[])
{
    static int (*real_execve)(const char*, char *const[], char *const[]) = NULL;
    if (real_execve == NULL)
        real_execve = dlsym(RTLD_NEXT, "execve");

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

    char **argv = malloc(sizeof(char*) * (count + 1));
    assert(argv);
    va_start(ap, arg);
    for (int i = 0; i < count; ++i)
        argv[i] = va_arg(ap, char*);
    argv[count] = NULL;
    va_end(ap);

    int result = execv(path, argv);

    free(argv);
    return result;
}

int execlp(const char *file, const char *arg, ...)
{
    va_list ap;

    va_start(ap, arg);
    int count = 0;
    while (va_arg(ap, const char*) != NULL)
        count++;
    va_end(ap);

    char **argv = malloc(sizeof(char*) * (count + 1));
    assert(argv);
    va_start(ap, arg);
    for (int i = 0; i < count; ++i)
        argv[i] = va_arg(ap, char*);
    argv[count] = NULL;
    va_end(ap);

    int result = execvp(file, argv);

    free(argv);
    return result;
}

int execle(const char *path, const char *arg, ...)
{
    va_list ap;

    va_start(ap, arg);
    int count = 0;
    while (va_arg(ap, const char*) != NULL)
        count++;
    va_end(ap);

    char **argv = malloc(sizeof(char*) * (count + 1));
    assert(argv);
    va_start(ap, arg);
    for (int i = 0; i < count; ++i)
        argv[i] = va_arg(ap, char*);
    argv[count] = NULL;
    char **envp = va_arg(ap, char**);
    va_end(ap);

    int result = execve(path, argv, envp);

    free(argv);
    return result;
}
