#ifndef NAV_MAINWINDOW_H
#define NAV_MAINWINDOW_H

#include <QFontMetricsF>
#include <QMainWindow>
#include <QMap>

#include "TextWidthCalculator.h"


namespace Nav {

namespace Ui {
class MainWindow;
}

class File;
class MainWindow;
class Project;
class Ref;
class SourceWidget;

extern MainWindow *theMainWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Project &project, QWidget *parent = 0);
    ~MainWindow();
    void navigateToFile(File *file);
    void navigateToRef(const Ref &ref);
    TextWidthCalculator &getCachedTextWidthCalculator(const QFont &font);

private slots:
    void actionViewFileList();
    void actionOpenGotoWindow();
    void actionCommand(const QString &command);

protected:
    void closeEvent(QCloseEvent *event);

private:
    QMap<QFont, TextWidthCalculator*> m_textWidthCalculatorCache;
    Ui::MainWindow *ui;
    SourceWidget *m_sourceWidget;
};

} // namespace Nav

#endif // NAV_MAINWINDOW_H
