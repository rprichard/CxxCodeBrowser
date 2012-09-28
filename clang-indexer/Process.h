#ifndef INDEXER_PROCESS_H
#define INDEXER_PROCESS_H

#include <cstdio>
#include <mutex>
#include <string>
#include <vector>

namespace indexer {

class ProcessPrivate;

// A limited process abstraction, useful for running the clang-indexer daemon
// and probably not much else.  It's similar to popen, but there are *two* pipe
// handles instead of just one.  Both of them do blocking I/O, so it's
// necessary to alternate between parent->child and child->parent
// communication, but that's fine for the daemon.
class Process
{
public:
    Process(const std::string &programPath,
            const std::vector<std::string> &args);
    ~Process();
    FILE *stdin() { return m_stdin; }
    FILE *stdout() { return m_stdout; }
    void closeStdin();
    void closeStdout();
    int wait();
    static std::mutex &creationMutex() { return m_creationMutex; }
private:
    ProcessPrivate *m_p;
    FILE *m_stdin;
    FILE *m_stdout;
    static std::mutex m_creationMutex;
};

} // namespace indexer

#endif // INDEXER_PROCESS_H
