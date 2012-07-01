#include <unistd.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>

static void logExecution(const char *filename, char *const argv[])
{
    char *logFile = getenv("BTRACE_LOG");
    if (logFile != NULL && logFile[0] != '\0') {
        FILE *fp = fopen(logFile, "a");
        flock(fileno(fp), LOCK_EX);
        int count = 0;
        for (int i = 0; argv[i] != NULL; ++i)
            count = i + 1;
        fprintf(fp, "%d\n", count + 1);
        fprintf(fp, "%s\n", filename);
        for (int i = 0; argv[i] != NULL; ++i)
            fprintf(fp, "%s\n", argv[i]);
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
