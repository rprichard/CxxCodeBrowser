#include "FileManager.h"

#include <QFile>
#include <QDirModel>
#include <cassert>

#include "File.h"
#include "Folder.h"

// XXX: Windows support
#define ROOT_PATH "/"
#define PATH_SEP '/'

namespace Nav {

// TODO: How canonicalized are paths within the index?  They can't have
// ".." in them -- but are symlinks resolved?

FileManager::FileManager(
        const QString &projectRootPath,
        const QList<QString> &indexPaths)
{
    m_categoryProject = new Folder(NULL, "", "Project");
    m_categoryOutside = new Folder(NULL, "", "Outside");
    m_categorySpecial = new Folder(NULL, "", "Special");
    m_allItems.append(m_categoryProject);
    m_allItems.append(m_categoryOutside);
    m_allItems.append(m_categorySpecial);

    m_dirProject = new Folder(
                m_categoryProject,
                QDir(projectRootPath).canonicalPath(),
                QDir(projectRootPath).dirName());
    m_allItems.append(m_dirProject);
    m_categoryProject->appendFolder(m_dirProject);
    m_dirFilesystem = new Folder(
                m_categoryOutside,
                ROOT_PATH,  // XXX: Won't work on Windows
                ROOT_PATH); // XXX: Won't work on Windows
    m_allItems.append(m_dirFilesystem);
    m_categoryOutside->appendFolder(m_dirFilesystem);

    foreach (const QString &path, indexPaths) {
        file(path);
    }

    m_categoryProject->sort();
    m_categoryOutside->sort();
    m_categorySpecial->sort();
}

QList<Folder*> FileManager::roots()
{
    QList<Folder*> result;
    result << m_categoryProject;
    if (!m_categoryOutside->isEmpty())
        result << m_categoryOutside;
    if (!m_categorySpecial->isEmpty())
        result << m_categorySpecial;
    return result;
}

FileManager::~FileManager()
{
    qDeleteAll(m_allItems);
}

File &FileManager::file(const QString &path)
{
    if (path.startsWith("/")) {
        QString projectPrefix = m_dirProject->path() + PATH_SEP;
        if (path.startsWith(projectPrefix)) {
            return file(m_dirProject, path.mid(projectPrefix.size()));
        } else {
            return file(m_dirFilesystem, path.mid(1));
        }
    } else {
        if (m_specialFiles.contains(path))
            return *m_specialFiles[path];
        File *result = new File(m_categorySpecial, path);
        m_allItems.append(result);
        m_specialFiles[path] = result;
        m_categorySpecial->appendFile(result);
        return *result;
    }
}

File &FileManager::file(Folder *folder, const QString &relativePath)
{
    int pathSepIndex = relativePath.indexOf(PATH_SEP);
    if (pathSepIndex == -1) {
        File *result;
        FolderItem *item = folder->get(relativePath);
        if (item != NULL) {
            assert(!item->isFolder());
            result = static_cast<File*>(item);
        } else {
            QString fullPath = folder->path() + PATH_SEP + relativePath;
            result = new File(folder, fullPath);
            m_allItems.append(result);
            folder->appendFile(result);
        }
        return *result;
    } else {
        QString part = relativePath.mid(0, pathSepIndex);
        Folder *result;
        FolderItem *item = folder->get(part);
        if (item != NULL) {
            assert(item->isFolder());
            result = static_cast<Folder*>(item);
        } else {
            QString fullPath = folder->path();
            if (!fullPath.endsWith(PATH_SEP))
                fullPath += PATH_SEP;
            fullPath += part;
            result = new Folder(folder, fullPath, part);
            m_allItems.append(result);
            folder->appendFolder(result);
        }
        return file(result, relativePath.mid(pathSepIndex + 1));
    }
}

} // namespace Nav
