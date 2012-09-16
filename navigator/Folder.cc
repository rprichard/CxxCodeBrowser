#include "Folder.h"

#include "File.h"

namespace Nav {

Folder::Folder(Folder *parent, const QString &path, const QString &title) :
    m_parent(parent), m_path(path), m_title(title)
{
}

void Folder::appendFolder(Folder *child)
{
    m_folders.append(child);
    m_items[child->title()] = child;
}

void Folder::appendFile(File *child)
{
    m_files.append(child);
    m_items[child->title()] = child;
}

FolderItem *Folder::get(const QString &title)
{
    auto it = m_items.find(title);
    if (it == m_items.end())
        return NULL;
    else
        return *it;
}

void Folder::sort()
{
    struct FolderItemPtrLessThan {
        bool operator()(FolderItem *x, FolderItem *y) const {
            return x->title() < y->title();
        }
    };
    qSort(m_folders.begin(), m_folders.end(), FolderItemPtrLessThan());
    qSort(m_files.begin(), m_files.end(), FolderItemPtrLessThan());
    foreach (Folder *folder, m_folders)
        folder->sort();
}

} // namespace Nav
