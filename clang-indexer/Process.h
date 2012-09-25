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
//
// I decided to create this class instead of using QProcess.  QProcess's API
// was confusing to me.  For example, does waitForReadyReady unblock if the
// process dies?  It's not "ready-to-read", but the input pipe should have been
// closed.  Closing the input pipe will unblock a native read/ReadFile call.
// How do waitForReadyRead and readLine interact?  I would prefer to have a
// readLine call that blocks until it reads one line, but does
// QProcess::readLine block?  It looks like it doesn't?  What if there's a
// partial line in the input?  Will readLine return as much of the line as it
// can?  I don't want that, but what if it doesn't return anything?  It seems
// that waitForReadyRead never blocks if there are bytes available to read,
// but I would have guessed the opposite from the documentation.
//
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
