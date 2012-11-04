#ifndef NAV_PROJECTINL_H
#define NAV_PROJECTINL_H

#include <stdint.h>

#include "../libindexdb/IndexDb.h"
#include "File.h"
#include "Project.h"
#include "Ref.h"

namespace Nav {

template <typename Func>
void Project::queryFileRefs(
        File &file,
        Func callback,
        uint32_t firstLine,
        uint32_t lastLine)
{
    indexdb::Row rowLookup(2);
    assert(RC_File == 0);
    assert(RC_Line == 1);
    rowLookup[RC_File] = fileID(file.path());
    rowLookup[RC_Line] = firstLine;
    // TODO: Add a class named TableIteratorRange (or TableRange) and a method
    // that accepts a lower bound and an upper bound.  It should do a single
    // O(log n) binary search, but produce two iterators.  This will
    // drastically simplify all of the querying code in this class.
    indexdb::TableIterator it = m_refTable->lowerBound(rowLookup);

    indexdb::TableIterator itEnd = m_refTable->end();
    indexdb::Row rowItem(RC_Count);
    for (; it != itEnd; ++it) {
        it.value(rowItem);
        // TODO: See above comment regarding TableIteratorRange.  This ought to
        // be removed.
        if (rowItem[RC_File] != rowLookup[RC_File] ||
                rowItem[RC_Line] > lastLine)
            break;
        Ref ref(*this,
                rowItem[RC_Symbol],
                rowItem[RC_File],
                rowItem[RC_Line],
                rowItem[RC_StartColumn],
                rowItem[RC_EndColumn],
                rowItem[RC_RefType]);
        callback(ref);
    }
}

} // namespace Nav

#endif // NAV_PROJECTINL_H
