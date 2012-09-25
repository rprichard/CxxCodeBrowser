#ifndef INDEXER_DAEMONPOOL_H
#define INDEXER_DAEMONPOOL_H

#include <QTextStream>
#include <QProcess>
#include <mutex>
#include <string>
#include <vector>

namespace indexer {

class Process;


///////////////////////////////////////////////////////////////////////////////
// Daemon

class Daemon
{
    friend class DaemonPool;
private:
    Daemon();
    ~Daemon();
public:
    int run(const std::string &workingDirectory,
            const std::vector<std::string> &args);
private:
    Process *m_process;
};


///////////////////////////////////////////////////////////////////////////////
// DaemonPool

class DaemonPool
{
public:
    DaemonPool();
    ~DaemonPool();
    Daemon *get();
    void release(Daemon *daemon);

private:
    std::mutex m_mutex;
    std::vector<Daemon*> m_daemons;
};

} // namespace indexer

#endif // INDEXER_DAEMONPOOL_H
