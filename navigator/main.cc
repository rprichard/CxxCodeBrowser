#include <QtGui/QApplication>
#include "NavMainWindow.h"
#include "SourceIndexer.h"
#include "Project.h"
#include "SourcesJsonReader.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Nav::theProject = Nav::readSourcesJson("/home/rprichard/project/test.sources");
    Nav::indexProject(Nav::theProject);

    theMainWindow = new NavMainWindow();
    theMainWindow->show();

    return a.exec();
}
