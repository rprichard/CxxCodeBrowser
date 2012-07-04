#ifndef NAV_TABLESUPPLIERSOURCELIST_H
#define NAV_TABLESUPPLIERSOURCELIST_H

#include "TableSupplier.h"

namespace Nav {

class TableSupplierSourceList : public Nav::TableSupplier
{
public:
    virtual std::vector<std::string> getColumnLabels();
    virtual std::vector<std::vector<std::string> > getData();
    virtual void select(const std::string &entry);
};

} // namespace Nav

#endif // NAV_TABLESUPPLIERSOURCELIST_H
