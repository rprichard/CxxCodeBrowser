#ifndef NAV_FOLDERITEM_H
#define NAV_FOLDERITEM_H

#include <QString>

namespace Nav {

class File;
class Folder;

struct FolderItem
{
    virtual ~FolderItem() {}
    virtual Folder *parent() = 0;
    virtual bool isFolder() = 0;
    virtual File *asFile() { return NULL; }
    virtual Folder *asFolder() { return NULL; }
    virtual QString path() = 0;
    virtual QString title() = 0;
};

} // namespace Nav

#endif // NAV_FOLDERITEM_H
