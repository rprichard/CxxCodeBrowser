#ifndef NAV_FILEMANAGER_H
#define NAV_FILEMANAGER_H

#include <QDir>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

namespace Nav {

class File;
class Folder;
class FolderItem;

class FileManager
{
public:
    FileManager(const QString &projectRootPath, const QStringList &indexPaths);
    ~FileManager();
    File &file(const QString &path);
    QList<Folder*> roots();

private:
    File &file(Folder *folder, const QString &relativePath);

private:
    // Root folders.  These folders act like category headings in
    // the hierarchical selected file pane.
    Folder *m_categorySpecial;  // special buffers like <builtins>
    Folder *m_categoryProject;
    Folder *m_categoryOutside;

    // The actual project and outside root folder.
    Folder *m_dirProject;
    Folder *m_dirFilesystem;

    QMap<QString, File*> m_specialFiles;
    QList<FolderItem*> m_allItems;
};

} // namespace Nav

#endif // NAV_FILEMANAGER_H
