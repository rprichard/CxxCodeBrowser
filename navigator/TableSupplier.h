#ifndef NAV_TABLESUPPLIER_H
#define NAV_TABLESUPPLIER_H

#include <string>
#include <vector>

namespace Nav {

class TableSupplier
{
public:
    virtual std::vector<std::string> getColumnLabels() = 0;
    virtual std::vector<std::vector<std::string> > getData() = 0;
    virtual void select(const std::string &entry) {}
    virtual void activate(const std::string &entry) {}
};

} // namespace Nav

#endif // NAV_TABLESUPPLIER_H
