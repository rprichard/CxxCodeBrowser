#include "FileManager.h"

#include <QDirModel>
#include <QFile>
#include <QList>
#include <QString>
#include <QStringList>
#include <cassert>
#include <QDebug>
#include "File.h"
#include "Folder.h"
#include <QStorageInfo>

//TODO resolve different root paths on windows
#if defined(SOURCEWEB_UNIX)
#define ROOT_PATH "/"
#define PATH_SEP '/'
#elif defined(_WIN32)
#define ROOT_PATH "C:\\"
#define PATH_SEP '\\'
#endif
// XXX: Windows support


namespace Nav {

// TODO: How canonicalized are paths within the index?  They can't have
// ".." in them -- but are symlinks resolved?

FileManager::FileManager(
        const QString &projectRootPath,
        const QStringList &indexPaths)
{

    //normalize project root path
    QString normalizedProjectRootPath = QDir::toNativeSeparators(projectRootPath);
    m_categoryProject = new Folder(NULL, "", "Project");
    m_categoryOutside = new Folder(NULL, "", "External");
    m_categorySpecial = new Folder(NULL, "", "Special");
    m_allItems.append(m_categoryProject);
    m_allItems.append(m_categoryOutside);
    m_allItems.append(m_categorySpecial);

    m_dirProject = new Folder(
                m_categoryProject,
                QDir::toNativeSeparators(QDir(normalizedProjectRootPath).canonicalPath()),
                QDir(normalizedProjectRootPath).dirName());
    m_allItems.append(m_dirProject);
    m_categoryProject->appendFolder(m_dirProject);
    //goes through each mounted drive on system, and adds its name to winRootPaths list, and creates Nav::Folder
    //which is appended to m_categoryOutside, and then pointer is added to m_dirFilesystemRoots list
    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes()) {
            if (storage.isValid() && storage.isReady()) {
                if (!storage.isReadOnly()) {
                    m_winRootPaths << QDir::toNativeSeparators(storage.displayName());

                    m_dirFilesystem = new Folder(
                                m_categoryOutside,
                                storage.displayName().mid(0,storage.displayName().length()-1),  // XXX: Won't work on Windows
                                storage.displayName()); // XXX: Won't work on Windows
                    m_allItems.append(m_dirFilesystem);
                    m_categoryOutside->appendFolder(m_dirFilesystem);
                    m_dirFilesystemRoots.append(m_dirFilesystem);
                }
            }
        }

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

//support for windows paths added into this function
//This is a quick workaround, its far from perfect, but it does the decent job
File &FileManager::file(const QString &path)
{
    Nav::Folder *currentFolder; //pointer to folder file belongs
    QString currentPath; //path to current file
    bool specialFlag = false; //becomes true if current file belongs to special category
    QString normalizedPath = QDir::toNativeSeparators(path);
    int iterator = 0; //iterator for filesystemRoots pointer list
    foreach (QString rootPath, m_winRootPaths) {
        if (normalizedPath.startsWith("/") || normalizedPath.startsWith(rootPath) ) {
            QString projectPrefix = QDir::toNativeSeparators(m_dirProject->path() + PATH_SEP);
            if (normalizedPath.startsWith(projectPrefix)) {
                currentFolder = m_dirProject;
                currentPath = normalizedPath.mid(projectPrefix.size());
                specialFlag = false;
                break;
            } else {
                currentFolder = m_dirFilesystemRoots.at(iterator);
                currentPath = normalizedPath.mid(QString(rootPath).length());
                specialFlag = false;
                break;
            }
        } else {
            specialFlag = true;
        }
        iterator++;
    }
    if (specialFlag){
        if (m_specialFiles.contains(path))
            return *m_specialFiles[path];
        File *result = new File(m_categorySpecial, path);
        m_allItems.append(result);
        m_specialFiles[path] = result;
        m_categorySpecial->appendFile(result);
        return *result;
    }
    else
        return file(currentFolder, currentPath);
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
