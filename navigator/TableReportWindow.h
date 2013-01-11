#ifndef NAV_TABLEREPORTWINDOW_H
#define NAV_TABLEREPORTWINDOW_H

#include <QEvent>
#include <QKeyEvent>
#include <QObject>
#include <QWidget>

namespace Nav {

class PlaceholderLineEdit;
class TableReport;
class TableReportView;

class TableReportWindow : public QWidget
{
    Q_OBJECT
public:
    explicit TableReportWindow(QWidget *parent = 0);
    void setTableReport(TableReport *report);
    void setFilterBoxVisible(bool visible);

private:
    bool eventFilter(QObject *object, QEvent *event);
    void keyPressEvent(QKeyEvent *event);

private slots:
    void filterTextChanged();

private:
    PlaceholderLineEdit *m_filterBox;
    TableReportView *m_view;
};

} // namespace Nav

#endif // NAV_TABLEREPORTWINDOW_H
