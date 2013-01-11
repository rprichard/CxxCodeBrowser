#include "ReportRefList.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtAlgorithms>
#include <cassert>
#include <cstring>
#include <string>
#include <stdint.h>

#include "File.h"
#include "MainWindow.h"
#include "Project.h"
#include "Ref.h"
#include "TableReport.h"

namespace Nav {

static void uint32ToString(char *output, uint32_t val)
{
    char tempBuf[32];
    char *ptr = &tempBuf[31];
    *ptr = '\0';
    do {
        *(--ptr) = '0' + val % 10;
        val /= 10;
    } while (val != 0);
    strcpy(output, ptr);
}

ReportRefList::ReportRefList(
        Project &project,
        const QString &symbol,
        QObject *parent) :
    TableReport(parent),
    m_symbol(symbol)
{
    m_refList = project.queryReferencesOfSymbol(symbol);
    qSort(m_refList.begin(), m_refList.end());
}

QString ReportRefList::title()
{
    return "References to " + m_symbol;
}

QStringList ReportRefList::columns()
{
    QStringList result;
    result << "File";
    result << "Line";
    result << "Type";
    return result;
}

int ReportRefList::rowCount()
{
    return m_refList.size();
}

const char *ReportRefList::text(int row, int col, std::string &tempBuf)
{
    const Ref &ref = m_refList[row];
    if (col == 0) {
        return ref.fileNameCStr();
    } else if (col == 1) {
        tempBuf.resize(32);
        uint32ToString(&tempBuf[0], ref.line());
        return &tempBuf[0];
    } else if (col == 2) {
        return ref.kindCStr();
    } else {
        assert(false && "Invalid column");
    }
}

void ReportRefList::select(int row)
{
    theMainWindow->navigateToRef(m_refList[row]);
}

int ReportRefList::compare(int row1, int row2, int col)
{
    const Ref &ref1 = m_refList[row1];
    const Ref &ref2 = m_refList[row2];
    if (col == 0) {
        return static_cast<int>(ref1.fileID()) -
                static_cast<int>(ref2.fileID());
    } else if (col == 1) {
        return ref1.line() - ref2.line();
    } else if (col == 2) {
        return static_cast<int>(ref1.kindID()) -
                static_cast<int>(ref2.kindID());
    } else {
        assert(false && "Invalid column");
    }
}

} // namespace Nav
