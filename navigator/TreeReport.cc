#include "TreeReport.h"
#include <QtGlobal>

namespace Nav {

int TableTreeReport::getChildCount(const Index &index)
{
    return index.isValid() ? 0 : getRowCount();
}

TableTreeReport::Index TableTreeReport::getChildIndex(const Index &parent, int row)
{
    Q_ASSERT(!parent.isValid());
    return getRowIndex(row);
}

} // namespace Nav
