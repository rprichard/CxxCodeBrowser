#include "Process.h"

#include <cassert>
#include <cstring>
#include <mutex>

#ifdef __unix__
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "Util.h"

namespace indexer {

#ifdef __unix__
struct ProcessPrivate
{
    pid_t pid;
    bool reaped;
    int status;
};
#endif

// To avoid leaking open files to subprocesses, grab this mutex before creating
// inheritable files and release it after marking the file
// non-inheritable/O_CLOEXEC (or after closing it).
std::mutex Process::m_creationMutex;

#ifdef __unix__
static inline void writeError(const char *str)
{
    size_t __attribute__((unused)) dummy;  // Silence compiler warning.
    dummy = write(STDERR_FILENO, str, strlen(str));
}
#endif

Process::Process(
        const std::string &programPath,
        const std::vector<std::string> &args)
    : m_p(new ProcessPrivate)
{
#ifdef __unix__
    memset(m_p, 0, sizeof(*m_p));
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
    // The const_cast is safe.  (http://stackoverflow.com/questions/190184/execv-and-const-ness)
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
        writeError("clang-indexer: Failed to exec ");
        writeError(programPathPtr);
        writeError("\n");
        _Exit(1);
    } else {
        // Parent process.
        m_creationMutex.unlock();
        m_stdin = fdopen(pipes[1], "w");
        m_stdout = fdopen(pipes[2], "r");
        close(pipes[0]);
        close(pipes[3]);
    }
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
#ifdef __unix__
    if (!m_p->reaped) {
        m_p->status = -1;
        EINTR_LOOP(waitpid(m_p->pid, &m_p->status, 0));
        m_p->reaped = true;
    }
    return m_p->status;
#else
#error "Process::wait not implemented on this OS."
#endif
}

void Process::closeStdin()
{
    if (m_stdin != NULL) {
        fclose(m_stdin);
        m_stdin = NULL;
    }
}

void Process::closeStdout()
{
    if (m_stdout != NULL) {
        fclose(m_stdout);
        m_stdout = NULL;
    }
}

} // namespace indexer
