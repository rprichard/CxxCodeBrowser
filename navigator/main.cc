#include <QTextCodec>
#include <QtGui/QApplication>

#include "MainWindow.h"
#include "Misc.h"
#include "Project.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    Nav::hackSwitchIconThemeToTheOneWithIcons();

    Nav::theProject = new Nav::Project("index");

    Nav::theMainWindow = new Nav::MainWindow(*Nav::theProject);
    Nav::theMainWindow->show();

    int result = a.exec();

    delete Nav::theProject;

    return result;
}
