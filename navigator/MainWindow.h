#ifndef NAV_MAINWINDOW_H
#define NAV_MAINWINDOW_H

#include <QMainWindow>

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

private slots:
    void actionViewFileList();
    void actionOpenGotoWindow();
    void actionCommand(const QString &command);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    SourceWidget *m_sourceWidget;
};

} // namespace Nav

#endif // NAV_MAINWINDOW_H
