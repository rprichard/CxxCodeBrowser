#ifndef NAV_FOLDER_H
#define NAV_FOLDER_H

#include <QList>
#include <QMap>
#include <QString>

#include "FolderItem.h"

namespace Nav {

class File;

class Folder : public FolderItem
{
    friend class FileManager;

private:
    Folder(Folder *parent, const QString &path, const QString &title);
    void appendFolder(Folder *child);
    void appendFile(File *child);
    void sort();

public:
    Folder *parent() { return m_parent; }
    bool isFolder() { return true; }
    Folder *asFolder() { return this; }
    QString path() { return m_path; }
    QString title() { return m_title; }
    FolderItem *get(const QString &title);
    bool isEmpty() { return m_items.isEmpty(); }
    QList<Folder*> folders() { return m_folders; }
    QList<File*> files() { return m_files; }

private:
    Folder *m_parent;
    QString m_path;
    QString m_title;
    QMap<QString, FolderItem*> m_items;
    QList<Folder*> m_folders;
    QList<File*> m_files;
};

} // namespace Nav

#endif // NAV_FOLDER_H
