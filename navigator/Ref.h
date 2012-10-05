#ifndef NAV_REF_H
#define NAV_REF_H

#include <QString>

#include "FileManager.h"
#include "Project.h"
#include "../libindexdb/IndexDb.h"

namespace Nav {

class File;
class Project;

class Ref
{
    friend bool operator==(const Ref &x, const Ref &y);
    friend bool operator<(const Ref &x, const Ref &y);

public:
    Ref() :
        m_project(NULL),
        m_symbolID(indexdb::kInvalidID),
        m_fileID(indexdb::kInvalidID),
        m_line(-1),
        m_column(-1),
        m_kindID(indexdb::kInvalidID)
    {
    }

    Ref(Project &project,
            indexdb::ID symbolID,
            indexdb::ID fileID,
            int line,
            int column,
            indexdb::ID kindID) :
        m_project(&project),
        m_symbolID(symbolID),
        m_fileID(fileID),
        m_line(line),
        m_column(column),
        m_kindID(kindID)
    {
    }

    bool isNull() const { return m_project == NULL; }

    const char *symbolCStr() const {
        return m_project->symbolStringTable().item(m_symbolID);
    }

    QString symbol() const { return symbolCStr(); }

    File &file() const {
        QString fileName = m_project->fileName(m_fileID);
        return m_project->fileManager().file(fileName);
    }

    int line() const { return m_line; }
    int column() const { return m_column; }

    QString kind() const {
        return m_project->refTypeStringTable().item(m_kindID);
    }

private:
    Project *m_project;
    indexdb::ID m_symbolID;
    indexdb::ID m_fileID;
    int m_line;         // 1-based
    int m_column;       // 1-based
    indexdb::ID m_kindID;
};

bool operator<(const Ref &x, const Ref &y);

inline bool operator==(const Ref &x, const Ref &y)
{
    return &x.m_project == &y.m_project &&
            x.m_symbolID == y.m_symbolID &&
            x.m_fileID == y.m_fileID &&
            x.m_line == y.m_line &&
            x.m_column == y.m_column &&
            x.m_kindID == y.m_kindID;
}

inline bool operator!=(const Ref &x, const Ref &y)
{
    return !(x == y);
}

} // namespace Nav

#endif // NAV_REF_H
