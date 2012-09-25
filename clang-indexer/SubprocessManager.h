#ifndef INDEXER_SUBPROCESSMANAGER_H
#define INDEXER_SUBPROCESSMANAGER_H

#include <QObject>
#include <QProcess>
#include <list>
#include <string>
#include <vector>

namespace indexer {

// Creates clang-indexer subprocesses and dispatches jobs to them.
class SubprocessManager : public QObject
{
    Q_OBJECT

private:
    struct Job {
        std::string title;
        std::string workingDirectory;
        std::vector<std::string> args;
        bool *doneFlag;
    };

    struct Process {
        bool ending;
        QProcess *process;
        QByteArray jobOutput;
        bool *doneFlag;
    };

public:
    explicit SubprocessManager(
            const std::string &indexerArgv0,
            QObject *parent = 0);
    void spawnJob(
            const std::string &title,
            const std::string &workingDirectory,
            const std::vector<std::string> &args,
            bool *doneFlag);
    bool doWork();

private:
    int runningProcessCount();
    void sendJobToDaemonProcess(const Job &job, Process &p);
    void writeKillToDaemonStdin(QProcess *process);

private slots:
    void pollProcesses();

private:
    std::string m_indexerBinPath;
    std::list<Job> m_queuedJobs;
    std::list<Process> m_processes;
};

} // namespace indexer

#endif // INDEXER_SUBPROCESSMANAGER_H
