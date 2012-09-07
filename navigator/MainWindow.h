#ifndef NAV_MAINWINDOW_H
#define NAV_MAINWINDOW_H

#include <QMainWindow>

namespace Nav {

namespace Ui {
class MainWindow;
}

class MainWindow;
class SourceWidget;
class Project;
class File;

extern MainWindow *theMainWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Project &project, QWidget *parent = 0);
    ~MainWindow();
    void showFile(const QString &path);
    void selectIdentifier(int line, int column);

private slots:
    void actionViewFileList();
    void actionCommand(const QString &command);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    SourceWidget *m_sourceWidget;
};

} // namespace Nav

#endif // NAV_MAINWINDOW_H
