#ifndef NAVMAINWINDOW_H
#define NAVMAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
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
    
private slots:
    void actionViewSource();
    void actionCommand(const QString &command);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
};

#endif // NAVMAINWINDOW_H
