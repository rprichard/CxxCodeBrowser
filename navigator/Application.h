#ifndef NAV_APPLICATION_H
#define NAV_APPLICATION_H

#include <QApplication>
#include <QFont>
#include <QSettings>
#include <QString>

namespace Nav {

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv);
    QFont defaultFont();
    QFont sourceFont();
    static Application *instance() {
        return qobject_cast<Application*>(QApplication::instance());
    }
private:
    QFont configurableFont(
            const QString &name,
            const QString &defaultFace,
            int defaultSize,
            bool defaultMonospace);
    QSettings m_settings;
};

} // namespace Nav

#endif // NAV_APPLICATION_H
