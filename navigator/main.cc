#include <QtGui/QApplication>
#include "NavMainWindow.h"
#include "NavTableWindow.h"
#include "TableSupplierSourceList.h"
#include "Program.h"
#include "SourcesJsonReader.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Nav::theProgram = Nav::readSourcesJson("/home/rprichard/openssl-1.0.1c/btrace.sources");

    theMainWindow = new NavMainWindow();
    theMainWindow->show();

    return a.exec();
}
