#include "Process.h"
#include "../shared_headers/host.h"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>

#if defined(SOURCEWEB_UNIX)
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#include <windows.h>
#endif // _WIN32

#include "Mutex.h"
#include "Util.h"

namespace indexer {

#if defined(SOURCEWEB_UNIX)
struct ProcessPrivate {
    pid_t pid;
    bool reaped;
    int status;
};
#elif defined(_WIN32)
struct ProcessPrivate {
    HANDLE hproc;
    bool reaped;
    int status;
};
#endif

// To avoid leaking open files to subprocesses, grab this mutex before creating
// inheritable files and release it after marking the file
// non-inheritable/O_CLOEXEC (or after closing it).
Mutex Process::m_creationMutex;

#if defined(SOURCEWEB_UNIX)
static inline void writeError(const char *str)
{
    size_t __attribute__((unused)) dummy;  // Silence compiler warning.
    dummy = write(STDERR_FILENO, str, strlen(str));
}
#endif

#ifdef _WIN32
// Convert argc/argv into a Win32 command-line following the escaping convention
// documented on MSDN.  (e.g. see CommandLineToArgvW documentation)
// Copied from winpty project.
static std::string argvToCommandLine(const std::vector<std::string> &argv)
{
    std::string result;
    for (size_t argIndex = 0; argIndex < argv.size(); ++argIndex) {
        if (argIndex > 0)
            result.push_back(' ');
        const char *arg = argv[argIndex].c_str();
        const bool quote =
            strchr(arg, ' ') != NULL ||
            strchr(arg, '\t') != NULL ||
            *arg == '\0';
        if (quote)
            result.push_back('\"');
        int bsCount = 0;
        for (const char *p = arg; *p != '\0'; ++p) {
            if (*p == '\\') {
                bsCount++;
            } else if (*p == '\"') {
                result.append(bsCount * 2 + 1, '\\');
                result.push_back('\"');
                bsCount = 0;
            } else {
                result.append(bsCount, '\\');
                bsCount = 0;
                result.push_back(*p);
            }
        }
        if (quote) {
            result.append(bsCount * 2, '\\');
            result.push_back('\"');
        } else {
            result.append(bsCount, '\\');
        }
    }
    return result;
}

static std::string makeCommandLine(
        const std::string &programPath,
        const std::vector<std::string> &args)
{
    std::vector<std::string> argv;
    argv.push_back(programPath);
    argv.insert(argv.end(), args.begin(), args.end());
    return argvToCommandLine(argv);
}
#endif

Process::Process(
        const std::string &programPath,
        const std::vector<std::string> &args)
    : m_p(new ProcessPrivate)
{
    memset(m_p, 0, sizeof(*m_p));
#if defined(SOURCEWEB_UNIX)
    int pipes[4];
    m_creationMutex.lock();
    int ret;
    ret = pipe(pipes);
    assert(ret == 0);
    ret = pipe(pipes + 2);
    assert(ret == 0);
    for (int fd : pipes)
        fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
    std::vector<const char*> argv;
    argv.push_back(programPath.c_str());
    for (const std::string &arg : args)
        argv.push_back(arg.c_str());
    argv.push_back(NULL);
    // The const_cast is safe.
    // http://stackoverflow.com/questions/190184/execv-and-const-ness
    char *const *argvPtr = const_cast<char *const *>(argv.data());
    const char *programPathPtr = programPath.c_str();

    m_p->pid = fork();
    assert(m_p->pid != -1);
    if (m_p->pid == 0) {
        // Child process.
        //
        // After forking, any other threads in the program have disappeared,
        // potentially leaving global memory (e.g. the heap) in an
        // inconsistent/locked state, so we must avoid calling most C library
        // routines in the child process.
        dup2(pipes[0], STDIN_FILENO);
        dup2(pipes[3], STDOUT_FILENO);
        // Uncomment this line to merge stderr with stdout.  I suspect error
        // handling is improved if I leave these commented out, though, at
        // least for now.
        //dup2(pipes[3], STDERR_FILENO);
        close(pipes[1]);
        close(pipes[2]);
        execv(programPathPtr, argvPtr);
        writeError("sw-clang-indexer: Failed to exec ");
        writeError(programPathPtr);
        writeError("\n");
        _Exit(1);
    } else {
        // Parent process.
        m_creationMutex.unlock();
        m_stdinFile = fdopen(pipes[1], "w");
        m_stdoutFile = fdopen(pipes[2], "r");
        close(pipes[0]);
        close(pipes[3]);
    }
#elif defined(_WIN32)
    // Try leaving the handles non-inheritable and pass bInheritHandles=FALSE to
    // CreateProcess.  I don't know whether this is supposed to work, but it
    // seems to.  CreateProcess seems to decide that the standard handles must
    // be inherited no matter what.
    BOOL success;
    std::string cmdLine = makeCommandLine(programPath, args); 


//child process handles
    HANDLE hStdinRead = NULL;
    HANDLE hStdinWrite = NULL;
    HANDLE hStdoutRead = NULL;
    HANDLE hStdoutWrite = NULL;

//  set the inheritHandle flag so pipe handles are inherited
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    //create pipe for the child process STDOUT
    success =  CreatePipe(&hStdoutRead, &hStdoutWrite, &saAttr, 0);
    assert(success);
    // Ensure the read handle to the pipe for STDOUT is not inherited.
    success = SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0);
    assert(success);

    //create pipe for the child process STDIN
    success =  CreatePipe(&hStdinRead, &hStdinWrite, &saAttr, 0);
    assert(success);
    // Ensure the write handle to the pipe for STDIN is not inherited.
    success = SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0);
          assert(success);

//  create child process
    STARTUPINFOA sui;
    PROCESS_INFORMATION pi;

    memset(&sui, 0, sizeof(sui));
    memset(&pi, 0, sizeof(pi));

    sui.cb = sizeof(sui);
    sui.hStdInput = hStdinRead;
    sui.hStdOutput = hStdoutWrite;
    sui.hStdError = hStdoutWrite;
    sui.dwFlags |= STARTF_USESTDHANDLES;

    BOOL ret = CreateProcessA(
                programPath.c_str(),
                &cmdLine[0],
                NULL, NULL,
                /*bInheritHandles=*/TRUE,
                /*dwCreationFlags=*/0,
                /*lpEnvironment=*/NULL,
                /*lpCurrentDirectory=*/NULL,
                &sui, &pi);
    if (!ret) {
        fprintf(stderr, "sw-clang-indexer: Error starting daemon process\n");
        exit(1);
    }

    m_p->hproc = pi.hProcess;
    CloseHandle(pi.hThread);
    CloseHandle(hStdinRead);
    CloseHandle(hStdoutWrite);

    //open files for stdin and stdout
    int stdinFd = _open_osfhandle(reinterpret_cast<intptr_t>(hStdinWrite),
                                  _O_TEXT | _O_RDWR);
    int stdoutFd = _open_osfhandle(reinterpret_cast<intptr_t>(hStdoutRead),
                                   _O_TEXT | _O_RDONLY);
    assert(stdinFd != -1);
    assert(stdoutFd != -1);
    m_stdinFile = _fdopen(stdinFd, "wt");
    m_stdoutFile = _fdopen(stdoutFd, "rt");
    assert(m_stdinFile != NULL);
    assert(m_stdoutFile != NULL);
#else
#error "Process::Process not implemented on this OS."
#endif
}

Process::~Process()
{
    wait();
    delete m_p;
}

// Close pipes to the process, then wait for it to exit (i.e. reap it if it
// hasn't already been reaped), then return its status code.
int Process::wait()
{
    closeStdin();
    closeStdout();
#if defined(SOURCEWEB_UNIX)
    if (!m_p->reaped) {
        m_p->status = -1;
        pid_t ret = EINTR_LOOP(waitpid(m_p->pid, &m_p->status, 0));
        assert(ret == m_p->pid && "unexpected waitpid return value");
        m_p->reaped = true;
    }
    return m_p->status;
#elif defined(_WIN32)
    if (!m_p->reaped) {
        DWORD ret = WaitForSingleObject(m_p->hproc, INFINITE);
        assert(ret == WAIT_OBJECT_0 && "wait on child process HANDLE failed");
        DWORD exitCode = 0;
        BOOL success = GetExitCodeProcess(m_p->hproc, &exitCode);
        assert(success && "GetExitCodeProcess on child process failed");
        m_p->status = exitCode;
        m_p->reaped = true;
        CloseHandle(m_p->hproc);
        m_p->hproc = INVALID_HANDLE_VALUE;
    }
    return m_p->status;
#else
#error "Process::wait not implemented on this OS."
#endif
}

void Process::closeStdin()
{
    if (m_stdinFile != NULL) {
        fclose(m_stdinFile);
        m_stdinFile = NULL;
    }
}

void Process::closeStdout()
{
    if (m_stdoutFile != NULL) {
        fclose(m_stdoutFile);
        m_stdoutFile = NULL;
    }
}

} // namespace indexer
