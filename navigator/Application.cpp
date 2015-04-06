#include "Application.h"

#include <QApplication>
#include <QFileInfo>
#include <QFont>
#include <QFontInfo>
#include <QMessageBox>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <memory>

#include "MainWindow.h"
#include "Project.h"

// Qt seems to have different methods for selecting a monospace font.  No one
// method works on all combinations of operating systems and Qt versions.  (In
// particular, Qt4 and Qt5 differ.)  Try each method and use the first font
// that is fixed-pitch according to QFontInfo.
static QFont setMonospaceFlagOnFont(QFont baseFont)
{
    if (QFontInfo(baseFont).fixedPitch())
        return baseFont;
    QFont f;
#if QT_VERSION >= QT_VERSION_CHECK(4, 7, 0)
    // The QFont::Monospace style is new in Qt 4.7.0.
    f = baseFont;
    f.setStyleHint(QFont::Monospace);
    if (QFontInfo(f).fixedPitch())
        return f;
#endif
    f = baseFont;
    f.setStyleHint(QFont::Courier);
    if (QFontInfo(f).fixedPitch())
        return f;
    f = baseFont;
    f.setFixedPitch(true);
    if (QFontInfo(f).fixedPitch())
        return f;
    // None of the methods worked, so give up.
    return baseFont;
}

namespace Nav {

Application::Application(int &argc, char **argv) :
    QApplication(argc, argv),
    m_settings(/*organization=*/"SourceWeb")
{
    // Work around Qt bugginess.  With Qt 5.4 on OS X 10.10.2, dialog boxes
    // opened in main() have a frozen UI for several seconds, and the menu bar
    // doesn't update when the app isn't a bundle.  Work around this by
    // deferring initialization until the event loop has started.
    QTimer *single = new QTimer(this);
    connect(single, SIGNAL(timeout()), SLOT(finishStartup()));
    single->setSingleShot(true);
    single->setInterval(0);
    single->start();
}

void Application::finishStartup()
{
    bool seen_help = false;
    QString path = "index";

    // The user might open the application from a GUI (e.g. by double-clicking
    // the app bundle in the OS X Finder).  If that happens, we want to show
    // the error via a dialog, because stderr is being discarded.  For
    // simplicity, always use a dialog.

    if (arguments().length() == 2) {
        path = arguments()[1];
        if (path == "-h" || path == "--help" || path == "-help") {
            seen_help = true;
        }
    }

    if (arguments().length() > 2 || seen_help) {
        QString message = QString("Usage: %0 [index_file]").arg(arguments()[0]);
        QMessageBox::information(nullptr, "SourceWeb", message);
        exit();
        return;
    }

    if (!QFileInfo(path).isFile()) {
        QString message = QString("Index file `%0` does not exist.").arg(path);
        QMessageBox::critical(nullptr, "SourceWeb", message);
        exit(1);
        return;
    }

    Nav::theProject = std::unique_ptr<Nav::Project>(new Nav::Project(path));
    Nav::theMainWindow = new Nav::MainWindow(*Nav::theProject);
    Nav::theMainWindow->show();
}

QFont Application::defaultFont()
{
    return configurableFont(
                "default",
            #if defined(__APPLE__)
                "", 11,
            #elif defined(__linux__) || defined(__FreeBSD__)
                "", 9,
            #else
                "", 0,
            #endif
                false);
}

QFont Application::sourceFont()
{
    return configurableFont(
                "source",
            #if defined(__APPLE__)
                "Menlo", 10,
            #elif defined(__linux__) || defined(__FreeBSD__)
                "Monospace", 8,
            #elif defined(_WIN32)
                "Lucida Console", 8,
            #else
                "", 0,
            #endif
                true);
}

QFont Application::configurableFont(
        const QString &name,
        const QString &defaultFace,
        int defaultSize,
        bool defaultMonospace)
{
    bool ok = false;
    QString face = m_settings.value(name + "_font_face").toString();
    int size =  m_settings.value(name + "_font_size").toInt(&ok);
    QFont font;
    if (!face.isEmpty() && ok) {
        font.setFamily(face);
        font.setPointSize(size);
        return font;
    } else {
        if (!defaultFace.isEmpty())
            font.setFamily(defaultFace);
        if (defaultSize != 0)
            font.setPointSize(defaultSize);
        if (defaultMonospace)
            font = setMonospaceFlagOnFont(font);
        return font;
    }
}

} // namespace Nav
