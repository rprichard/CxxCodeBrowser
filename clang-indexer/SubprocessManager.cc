#include "SubprocessManager.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QThread>
#include <iostream>

namespace indexer {

SubprocessManager::SubprocessManager(
        const std::string &indexerArgv0,
        QObject *parent) :
    QObject(parent)
{
    QFileInfo fileInfo(QString::fromStdString(indexerArgv0));
    m_indexerBinPath = fileInfo.absoluteFilePath().toStdString();
}

void SubprocessManager::spawnJob(
        const std::string &title,
        const std::string &workingDirectory,
        const std::vector<std::string> &args,
        bool *doneFlag)
{
    // Create a new job.
    Job job;
    job.title = title;
    job.workingDirectory = workingDirectory;
    job.args = args;
    job.doneFlag = doneFlag;

    //std::cerr << "SPAWN: " << m_processes.size() << " " << runningProcessCount() << " " << QThread::idealThreadCount() << std::endl;
    //for (std::list<Process>::iterator it = m_processes.begin(); it != m_processes.end(); ++it)
    //    std::cerr << it->ending << std::endl;

    if (runningProcessCount() < std::max(1, QThread::idealThreadCount())) {
        // Spawn a daemon process.  Feed the new job to it.
        Process p;
        p.ending = false;
        p.process = new QProcess;
        p.process->setProcessChannelMode(QProcess::MergedChannels);
        connect(p.process, SIGNAL(readyReadStandardOutput()), this, SLOT(pollProcesses()));
        connect(p.process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(pollProcesses()));
        p.process->start(QString::fromStdString(m_indexerBinPath), QStringList("--daemon"));
        sendJobToDaemonProcess(job, p);
        m_processes.push_back(p);
    } else {
        // Queue the job for later.
        m_queuedJobs.push_back(job);
    }
}

// Do some work and return true when nothing is left to do.
bool SubprocessManager::doWork()
{
    QCoreApplication::instance()->exec();
    return m_processes.empty();
}

void SubprocessManager::pollProcesses()
{
    std::vector<std::list<Process>::iterator> cleanupList;

    // Check processes for completion.
    for (std::list<Process>::iterator it = m_processes.begin();
            it != m_processes.end(); ++it) {
        Process &p = *it;
        p.jobOutput += p.process->readAllStandardOutput();
        if (p.process->state() == QProcess::NotRunning) {
            if (p.ending) {
                // Expected subprocess exit.
                p.process->deleteLater();
                cleanupList.push_back(it);
            } else {
                std::cerr << "indexer: daemon subprocess died unexpectedly.  Output:\n" << std::endl;
                std::cerr << p.jobOutput.data() << std::endl;
                exit(1);
            }
        }
        // Scan the complete lines in the output, line-by-line, looking for
        // the DONE marker.
        int startOffset = 0;
        bool jobDone = false;
        while (true) {
            int eolOffset = p.jobOutput.indexOf('\n', startOffset);
            if (eolOffset == -1)
                break;
            if (!strncmp(p.jobOutput.data() + startOffset, "DONE ", 5)) {
                jobDone = true;
            }
            startOffset = eolOffset + 1;
        }
        // Discarded the completed, scanned lines.
        if (startOffset != 0)
            p.jobOutput = p.jobOutput.mid(startOffset);
        if (jobDone) {
            // Mark the job done.
            if (p.doneFlag != NULL)
                *p.doneFlag = true;
            // Either submit another job or kill the daemon.
            if (!m_queuedJobs.empty()) {
                sendJobToDaemonProcess(m_queuedJobs.front(), p);
                m_queuedJobs.pop_front();
            } else {
                p.ending = true;
                writeKillToDaemonStdin(p.process);
            }
            QCoreApplication::exit();
        }
    }

    for (std::list<Process>::iterator it : cleanupList) {
        Process &p = *it;
        m_processes.erase(it);
        p.process->deleteLater();
        QCoreApplication::exit();
    }
}

int SubprocessManager::runningProcessCount()
{
    int count = 0;
    for (const auto &p : m_processes) {
        if (!p.ending)
            count++;
    }
    return count;
}

void SubprocessManager::sendJobToDaemonProcess(
        const Job &job,
        Process &p)
{
    std::cout << job.title << std::endl;
    p.doneFlag = job.doneFlag;
    p.process->write(job.workingDirectory.data());
    p.process->write("\n");
    for (const std::string &arg : job.args) {
        // TODO: What about embedded newlines in arguments?
        p.process->write(arg.data());
        p.process->write("\n");
    }
    p.process->write("\n");
}

void SubprocessManager::writeKillToDaemonStdin(QProcess *process)
{
    process->write("\n");
}

} // namespace indexer
