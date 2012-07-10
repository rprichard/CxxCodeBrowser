#ifndef NAV_TREEREPORT_H
#define NAV_TREEREPORT_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QObject>

class QAbstractItemModel;
class QModelIndex;

namespace Nav {

class TreeReport
{
public:
    class Index {
        int m_row;
        void *m_internalPtr;
    public:
        Index(int row = -1, void *internalPtr = NULL) :
            m_row(row), m_internalPtr(internalPtr) {}
        int row() const { return m_row; }
        bool isValid() const { return m_row != -1; }
        void *internalPtr() const { return m_internalPtr; }
    };

    virtual ~TreeReport() {}
    virtual QString getTitle() { return "Tree"; }
    virtual QStringList getColumns() { return QStringList(); }
    virtual int getChildCount(const Index &parent) = 0;
    virtual Index getChildIndex(const Index &parent, int row) = 0;
    virtual QList<QVariant> getText(const Index &index) = 0;
    virtual void select(const Index &index) {}
    virtual void activate(const Index &index) {}
};

class TableReport : public TreeReport
{
public:
    int getChildCount(const Index &index);
    Index getChildIndex(const Index &parent, int row);
protected:
    virtual QString getTitle() { return "Table"; }
    virtual int getRowCount() = 0;
    virtual Index getRowIndex(int row) { return Index(row); }
};

} // namespace Nav

#endif // NAV_TREEREPORT_H
