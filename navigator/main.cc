#include <QtGui/QApplication>
#include "NavMainWindow.h"
#include "NavTableWindow.h"
#include "Program.h"
#include "SourcesJsonReader.h"
#include "TableSupplierSourceList.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Nav::theProgram = Nav::readSourcesJson("/home/rprichard/openssl-1.0.1c/btrace.sources");

    theMainWindow = new NavMainWindow();
    theMainWindow->show();

    Nav::TableSupplierSourceList *supplier = new Nav::TableSupplierSourceList();
    NavTableWindow *tw = new NavTableWindow(supplier);

    tw->show();

    return a.exec();
}
