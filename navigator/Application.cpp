#include "Application.h"

#include <QSettings>

namespace Nav {

Application::Application(int &argc, char **argv) :
    QApplication(argc, argv),
    m_settings(/*organization=*/"SourceWeb")
{
}

QFont Application::defaultFont()
{
    return configurableFont(
                "default",
            #ifdef __APPLE__
                "", 11
            #else
                "", 9
            #endif
                );
}

QFont Application::sourceFont()
{
    return configurableFont(
                "source",
            #ifdef __APPLE__
                "Menlo", 10
            #else
                "Monospace", 8
            #endif
                );
}

QFont Application::configurableFont(
        const QString &name,
        const QString &defaultFace,
        int defaultSize)
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
        return font;
    }
}

} // namespace Nav
