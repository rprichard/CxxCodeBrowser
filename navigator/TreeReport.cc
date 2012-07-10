#include "TreeReport.h"
#include <QtGlobal>

namespace Nav {

int TableReport::getChildCount(const Index &index)
{
    return index.isValid() ? 0 : getRowCount();
}

TableReport::Index TableReport::getChildIndex(const Index &parent, int row)
{
    Q_ASSERT(!parent.isValid());
    return getRowIndex(row);
}

} // namespace Nav
