#include "NavMainWindow.h"
#include "ui_NavMainWindow.h"
#include "Program.h"
#include "Source.h"
#include "SourcesJsonReader.h"
#include <QFile>

NavMainWindow *theMainWindow;

NavMainWindow::NavMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

NavMainWindow::~NavMainWindow()
{
    delete ui;
}

void NavMainWindow::showFile(const std::string &path)
{
    QFile f(QString::fromStdString(path));
    f.open(QIODevice::ReadOnly);
    ui->textEdit->setText(f.readAll());
    f.close();
}
