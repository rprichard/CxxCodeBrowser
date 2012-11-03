#include <QCoreApplication>
#include <QtCore>
#include <QtDebug>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#ifdef __unix__
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <json/reader.h>

#include "../libindexdb/IndexArchiveBuilder.h"
#include "../libindexdb/IndexArchiveReader.h"
#include "../libindexdb/IndexDb.h"
#include "DaemonPool.h"
#include "IndexBuilder.h"
#include "TUIndexer.h"
#include "Util.h"

namespace indexer {

// TODO: This needs to be configured somehow.
// The Clang driver uses this driver path to locate its built-in include files
// which are at ../lib/clang/<VERSION>/include from the bin directory.
#define kDriverPath "/home/rprichard/llvm-install/bin/clang"

static std::vector<std::string> splitCommandLine(const std::string &commandLine)
{
    // Just split it by spaces for now.
    // TODO: Get this actually right -- e.g. quotes, escapes, tabs, newlines
    // TODO: On Windows, do we follow the weird Windows command-line
    // convention?  Note that there are two known users of this format:  CMake
    // generates it with -DCMAKE_EXPORT_COMPILE_COMMANDS and Clang's libtooling
    // can consume it.  Both should be audited.
    std::vector<std::string> result;
    size_t start = 0;
    while (start < commandLine.size()) {
        size_t next = commandLine.find(' ', start);
        if (next == std::string::npos)
            next = commandLine.size();
        if (next > start)
            result.push_back(commandLine.substr(start, next - start));
        start = next + 1;
    }
    return result;
}

struct SourceFileInfo {
    std::string sourceFilePath;
    std::string workingDirectory;
    std::string indexFilePath;
    std::vector<std::string> clangArgv;
};

static void readSourcesJson(
        const Json::Value &json,
        std::vector<SourceFileInfo> &output)
{
    output.clear();

    for (Json::ValueIterator it = json.begin(), itEnd = json.end();
            it != itEnd; ++it) {
        Json::Value &sourceJson = *it;
        SourceFileInfo sfi;
        QDir workingDirectory(QString::fromStdString(
                                  sourceJson["directory"].asString()));
        sfi.workingDirectory = workingDirectory.absolutePath().toStdString();
        sfi.sourceFilePath =
                QFileInfo(workingDirectory,
                    QString::fromStdString(
                        sourceJson["file"].asString())).absoluteFilePath().toStdString();
        sfi.clangArgv = splitCommandLine(sourceJson["command"].asString());

        // Replace the first argument with the known Clang driver.
        if (sfi.clangArgv.size() >= 1) {
            // TODO: What if the argument is actually Clang, such as
            // /usr/bin/clang?  Actually, can we get away with just using the
            // compiler in the JSON file?
            bool isCXX = stringEndsWith(sfi.clangArgv[0], "++");
            sfi.clangArgv[0] = kDriverPath;
            if (isCXX)
                sfi.clangArgv[0] += "++";
        }

        // Scan the argv looking for an -o argument specifying the output
        // object file.  Make the path absolute and change the suffix to
        // idx, then use this as the output index file's path.
        for (size_t i = 0; i < sfi.clangArgv.size() - 1; ++i) {
            if (sfi.clangArgv[i] == "-o" &&
                    (stringEndsWith(sfi.clangArgv[i + 1], ".o") ||
                              stringEndsWith(sfi.clangArgv[i + 1], ".obj"))) {
                QFileInfo objFile(
                            workingDirectory,
                            QString::fromStdString(sfi.clangArgv[i + 1]));
                std::string filePath =
                        objFile.absoluteFilePath().toStdString();
                filePath.erase(filePath.begin() + filePath.rfind('.'),
                               filePath.end());
                filePath += ".idx";
                sfi.indexFilePath = filePath;
                break;
            }
        }

        // If there wasn't a -o argument, combine the working directory with
        // the basename of the source path, and change the extension to .idx.
        if (sfi.indexFilePath.empty()) {
            std::string basename = const_basename(sfi.sourceFilePath.c_str());
            auto pos = basename.rfind('.');
            if (pos != std::string::npos) {
                basename = basename.substr(0, pos) + ".idx";
            }
            QFileInfo fileInfo(
                        workingDirectory,
                        QString::fromStdString(basename));
            sfi.indexFilePath = fileInfo.absoluteFilePath().toStdString();
        }

        output.push_back(sfi);
    }
}

static void readSourcesJson(
        const std::string &filename,
        std::vector<SourceFileInfo> &output)
{
    std::ifstream f(filename.c_str());
    Json::Reader r;
    Json::Value rootJson;
    r.parse(f, rootJson);
    readSourcesJson(rootJson, output);
}

const time_t kInvalidTime = static_cast<time_t>(-1);

static time_t getPathModTime(const std::string &path)
{
#if defined(__unix__)
    // TODO: What about symlinks?  It seems that the perfect behavior is to use
    // the non-symlink modtime for the index file itself, but for input files,
    // to use the highest modtime among all the symlinks and the non-symlink.
    // That's complicated, though, so just use the modtime of the non-symlink.
    struct stat buf;
    if (stat(path.c_str(), &buf) != 0)
        return kInvalidTime;
    return buf.st_mtime;
#elif defined(_WIN32)
#error "Should use GetFileInformation on Win32 -- not implemented yet"
#else
#error "Not implemented on this OS."
#endif
}

static time_t getCachedPathModTime(
        std::unordered_map<std::string, time_t> &fileTimeCache,
        const std::string &path)
{
    auto it = fileTimeCache.find(path);
    if (it != fileTimeCache.end()) {
        return it->second;
    } else {
        time_t t = getPathModTime(path);
        fileTimeCache[path] = t;
        return t;
    }
}

// TODO: We need to save the (workingDirectory, clang-args) in the index file
// somehow, so that if they change, we don't reuse the index.
static bool canReuseExistingIndexFile(
        std::unordered_map<std::string, time_t> &fileTimeCache,
        const SourceFileInfo &sfi)
{
    if (sfi.indexFilePath.empty())
        return false;
    time_t indexTime = getPathModTime(sfi.indexFilePath);
    if (indexTime == kInvalidTime)
        return false;
    indexdb::IndexArchiveReader archive(sfi.indexFilePath);
    for (int i = 0, iEnd = archive.size(); i < iEnd; ++i) {
        std::string path = archive.entry(i).name;
        if (path[0] == '\0' || path[0] == '<')
            continue;
        time_t inputTime = getCachedPathModTime(fileTimeCache, path);
        if (inputTime == kInvalidTime || inputTime > indexTime)
            return false;
    }
    return true;
}

static std::string indexProjectFile(
        DaemonPool *daemonPool,
        SourceFileInfo *sfi)
{
    if (sfi->indexFilePath.empty()) {
        // TODO: In theory, these temporary files could take up an arbitrarily
        // large amount of disk space, if the indexer were to get ahead of the
        // merging step.  I'd like to enforce a cap on the number of temp files
        // existing at a given time, but I'd have to be careful not to cause a
        // deadlock considering that the merging happens in a deterministic
        // order, so progress must always be possible on the next file to
        // merge.  The indexing seems to be much slower than the merging step.
        QTemporaryFile tempFile;
        tempFile.setAutoRemove(false);
        // TODO: Is this temporary file opened O_CLOEXEC?
        tempFile.open();
        sfi->indexFilePath = tempFile.fileName().toStdString();
    }

    Daemon *daemon = daemonPool->get();
    std::vector<std::string> args;
    args.push_back("--index-file");
    args.push_back(sfi->indexFilePath);
    args.push_back("--");
    args.insert(args.end(), sfi->clangArgv.begin(), sfi->clangArgv.end());
    daemon->run(sfi->workingDirectory, args);
    daemonPool->release(daemon);
    return sfi->indexFilePath;
}

static std::string identityString(std::string str)
{
    return str;
}

static int indexProject(const std::string &argv0, bool incremental)
{
    std::vector<SourceFileInfo> sourceFiles;
    readSourcesJson(std::string("compile_commands.json"), sourceFiles);

    DaemonPool daemonPool;
    std::unique_ptr<indexdb::Index> mergedIndex(new indexdb::Index);
    std::vector<std::pair<std::string, QFuture<std::string> > > futures;
    std::unordered_map<std::string, time_t> fileTimeCache;

    for (auto &sfi : sourceFiles) {
        if (!incremental)
            sfi.indexFilePath = "";
        if (canReuseExistingIndexFile(fileTimeCache, sfi)) {
            // TODO: It's inefficient to run identityString on a separate
            // thread.
            QFuture<std::string> future = QtConcurrent::run(
                        identityString, sfi.indexFilePath);
            futures.push_back(std::make_pair(sfi.sourceFilePath, future));
        } else {
            QFuture<std::string> future = QtConcurrent::run(
                        indexProjectFile, &daemonPool, &sfi);
            futures.push_back(std::make_pair(sfi.sourceFilePath, future));
        }
    }

    std::unordered_set<std::string> mergedEntrySet;

    for (const auto &p : futures) {
        std::string indexPath = p.second.result();
        std::cout << "Indexed " << p.first << std::endl;
        {
            indexdb::IndexArchiveReader archive(indexPath);
            for (int i = 0; i < archive.size(); ++i) {
                if (mergedEntrySet.find(archive.entry(i).hash) !=
                        mergedEntrySet.end())
                    continue;
                mergedEntrySet.insert(archive.entry(i).hash);
                indexdb::Index *fileIndex = archive.openEntry(i);
                mergedIndex->merge(*fileIndex);
                delete fileIndex;
            }
        }
        if (!incremental)
            QFile(QString::fromStdString(indexPath)).remove();
    }

    mergedIndex->finalizeTables();
    {
        IndexBuilder locationPopulator(*mergedIndex);
        locationPopulator.populateIndexTables();
    }
    mergedIndex->finalizeTables();
    mergedIndex->write("index");

    return 0;
}

static int indexFile(
        const std::string &outputFile,
        const std::vector<std::string> &clangArgv)
{
    indexdb::IndexArchiveBuilder archive;
    indexTranslationUnit(clangArgv, archive);
    archive.finalize();
    archive.write(outputFile, /*compressed=*/true);
    return 0;
}

static int runCommand(const std::vector<std::string> &argv)
{
    const char *const kUsageTextPattern =
            //        1         2         3         4         5         6         7         8
            //        0         0         0         0         0         0         0         0
            "Usage: %s\n"
            "\n"
            "    --index-project [--incremental]\n"
            "          Index all of the translation units in the compile_commands.json file\n"
            "          and create a single merged index file named index.\n"
            "\n"
            "          If --incremental is specified, then each translation unit's index is\n"
            "          saved to a separate idx file, which is reused by later --index-project\n"
            "          invocations if none of its referenced files have changed.\n"
            "\n"
            "    --index-file index-out-file -- clang-path clang-arguments...\n"
            "          Index a single translation unit.  Write the index to index-out-file.\n"
            "          clang-path must be the full path to a clang or clang++ driver\n"
            "          executable.  (This executable is not invoked, but libclang uses its\n"
            "          path to locate header files like stdarg.h.)\n";

            // TODO: I suspect it also uses the clang vs clang++ to decide between the C and
            // C++ languages.  Verify whether that's the case, and if so, mention it because
            // it's the reason a driver basename is passed rather than just a path to the
            // include file directory.

    // TODO: Improve the argument parsing (allow --help anywhere, allow reversing the args)

    if (argv.size() >= 2 && argv[1] == "--index-project") {
        bool incremental = argv.size() >= 3 && argv[2] == "--incremental";
        return indexProject(argv[0], incremental);
    } else if (argv.size() >= 6 &&
               argv[1] == "--index-file" &&
               argv[3] == "--") {
        std::string outputFile = argv[2];
        std::vector<std::string> clangArgv = argv;
        clangArgv.erase(clangArgv.begin(), clangArgv.begin() + 4);
        return indexFile(outputFile, clangArgv);
    } else {
        printf(kUsageTextPattern, argv[0].c_str());
        return 0;
    }
}

// Read a series of commands from stdin and run them.  After each command,
// print a line "DONE <status-code>".  Daemon mode exists mostly to avoid
// process creation overhead on Windows.
//
// The input for each command is a series of lines, starting with a working
// directory line, followed by a line for each argument, followed by a blank
// line.  The master process kills the daemon by closing the stdin pipe.
//
// Example input:
//
//     --index-file
//     /tmp/hello1.idx
//     --
//     /tmp/hello1.c
//     -DFOO=BAR
//
//     --index-file
//     /tmp/hello2.idx
//     --
//     /tmp/hello2.c
//     -DFOO=BAR
//
static int runDaemon(const char *argv0)
{
    while (true) {
        std::string cwd = readLine(stdin);
        if (cwd.empty())
            return 0;
        std::vector<std::string> commandArgv;
        commandArgv.push_back(argv0);
        // TODO: What about newlines embedded in arguments?
        while (true) {
            std::string line = readLine(stdin);
            if (line.empty())
                break;
            commandArgv.push_back(line);
        }
        if (commandArgv.size() <= 1) {
            std::cerr << argv0 << " daemon error: "
                      << "command arguments missing." << std::endl;
            exit(1);
        }
        if (chdir(cwd.c_str()) != 0) {
            std::stringstream err;
            err << argv0 << " daemon error: chdir failed";
            perror(err.str().c_str());
            exit(1);
        }
        int statusCode = runCommand(commandArgv);
        printf("DONE %d\n", statusCode);
        fflush(stdout);
    }
}

} // namespace indexer

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc == 2 && !strcmp(argv[1], "--daemon")) {
        return indexer::runDaemon(argv[0]);
    } else {
        std::vector<std::string> commandArgv;
        for (int i = 0; i < argc; ++i)
            commandArgv.push_back(argv[i]);
        return indexer::runCommand(commandArgv);
    }
}
