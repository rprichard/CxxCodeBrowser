#ifndef NAVMAINWINDOW_H
#define NAVMAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

namespace Nav {
class File;
}

class NavMainWindow;

extern NavMainWindow *theMainWindow;

class NavMainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit NavMainWindow(QWidget *parent = 0);
    ~NavMainWindow();
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

#endif // NAVMAINWINDOW_H
