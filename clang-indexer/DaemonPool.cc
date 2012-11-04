#include "DaemonPool.h"

#include <QCoreApplication>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "Process.h"
#include "Util.h"

namespace indexer {


///////////////////////////////////////////////////////////////////////////////
// Daemon

Daemon::Daemon()
{
    std::string program =
            QCoreApplication::instance()->applicationFilePath().toStdString();
    std::vector<std::string> args;
    args.push_back("--daemon");
    m_process = new Process(program, args);
}

Daemon::~Daemon()
{
    fprintf(m_process->stdout(), "\n");
    delete m_process;
}

int Daemon::run(
        const std::string &workingDirectory,
        const std::vector<std::string> &args)
{
    // Send the job.
    fprintf(m_process->stdin(), "%s\n", workingDirectory.c_str());
    for (const std::string &arg : args) {
        // TODO: What about embedded newlines in arguments?
        fprintf(m_process->stdin(), "%s\n", arg.c_str());
    }
    fprintf(m_process->stdin(), "\n");
    fflush(m_process->stdin());

    // Wait for completion.
    while (true) {
        bool isEof;
        std::string line = readLine(m_process->stdout(), &isEof);
        if (isEof) {
            std::cerr << "clang-indexer: daemon exited unexpectedly" << std::endl;
            return 1;
        }
        if (stringStartsWith(line, "DONE ")) {
            return atoi(line.c_str() + 5);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// DaemonPool

DaemonPool::DaemonPool()
{
}

DaemonPool::~DaemonPool()
{
    for (Daemon *daemon : m_daemons)
        delete daemon;
}

Daemon *DaemonPool::get()
{
    LockGuard<Mutex> lock(m_mutex);
    if (!m_daemons.empty()) {
        Daemon *result = m_daemons.back();
        m_daemons.pop_back();
        return result;
    } else {
        return new Daemon();
    }
}

void DaemonPool::release(Daemon *daemon)
{
    LockGuard<Mutex> lock(m_mutex);
    m_daemons.push_back(daemon);
}

} // namespace indexer
