#include <QtGui/QApplication>
#include "MainWindow.h"
#include "Project.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Nav::theProject = new Nav::Project("index");

    Nav::theMainWindow = new Nav::MainWindow(*Nav::theProject);
    Nav::theMainWindow->show();

    return a.exec();
}
