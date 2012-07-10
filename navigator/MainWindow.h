#ifndef NAV_MAINWINDOW_H
#define NAV_MAINWINDOW_H

#include <QMainWindow>

namespace Nav {

namespace Ui {
class MainWindow;
}

class File;
class MainWindow;

extern MainWindow *theMainWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void showFile(const QString &path);
    void selectText(int line, int column, int size);

private slots:
    void actionViewSource();
    void actionCommand(const QString &command);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    Nav::File *file;
};

} // namespace Nav

#endif // NAV_MAINWINDOW_H
