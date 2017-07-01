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
/**
 * @brief Constructor of Daemon class
 */
Daemon::Daemon()
{
    std::string program =
            QCoreApplication::instance()->applicationFilePath().toStdString();
    std::vector<std::string> args;
    args.push_back("--daemon");
    m_process = new Process(program, args);
}
/**
 * @brief Destructor of Daemon class
 */
Daemon::~Daemon()
{
    fprintf(m_process->stdoutFile(), "\n");
    delete m_process;
}

/**
 * @brief Function sends cmd arguments to child process by writing to stdin file child reads
 * and then expects output from child process in stdout file,
 * after reciving "DONE 0" from child, function destroys child process and exits
 * @param workingDirectory
 * @param args
 * @return
 */
int Daemon::run(
        const std::string &workingDirectory,
        const std::vector<std::string> &args)
{
    // Send the job.
    fprintf(m_process->stdinFile(), "%s\n", workingDirectory.c_str());
    for (const std::string &arg : args) {
        // TODO: What about embedded newlines in arguments?
        fprintf(m_process->stdinFile(), "%s\n", arg.c_str());
    }
    fprintf(m_process->stdinFile(), "\n");
    fflush(m_process->stdinFile());

    // Wait for completion.
    while (true) {
        bool isEof;
        std::string line = readLine(m_process->stdoutFile(), &isEof);
        if (isEof) {
            std::cerr << "sw-clang-indexer: daemon exited unexpectedly"
                      << std::endl;
            return 1;
        }
        if (stringStartsWith(line, "DONE ")) {
            return atoi(line.c_str() + 5);
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// DaemonPool

/**
 * @brief DaemonPool::DaemonPool constructor
 */
DaemonPool::DaemonPool()
{
}
/**
 * @brief DaemonPool::~DaemonPool destructor
 */

DaemonPool::~DaemonPool()
{
    for (Daemon *daemon : m_daemons)
        delete daemon;
}
/**
 * @brief If daemon list is empty create new daemon, else return last daemon inside list
 * @return
 */
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
/**
 * @brief Removes last daemon from list
 * @param daemon
 */
void DaemonPool::release(Daemon *daemon)
{
    LockGuard<Mutex> lock(m_mutex);
    m_daemons.push_back(daemon);
}

} // namespace indexer
