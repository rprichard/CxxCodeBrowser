#include "Misc.h"

#include <QFontMetrics>
#include <QIcon>
#include <QString>
#include <algorithm>

namespace Nav {

// Sometimes the line spacing is smaller than the height, which makes the text
// cramped.  When this happens, use the height instead.  (I think Qt already
// does this -- look for QTextLayout and a negative leading()).
int effectiveLineSpacing(const QFontMetrics &fm)
{
    return std::max(fm.height(), fm.lineSpacing());
}

// On my Linux Mint 13 system, QIcon::themeName identifies my theme as
// "hicolor".  The preferences pane, on the other hand, says my icon theme is
// "MATE".  In any case, the /usr/share/icons/hicolor directory lacks most of
// the icons I care about, so if I don't see the icons I want, I'm going to try
// switching the icon theme to gnome.
void hackSwitchIconThemeToTheOneWithIcons()
{
    QString originalThemeName = QIcon::themeName();
    QIcon previous = QIcon::fromTheme("go-previous");
    QIcon next = QIcon::fromTheme("go-next");
    if (previous.isNull() && next.isNull()) {
        QIcon::setThemeName("gnome");
        previous = QIcon::fromTheme("go-previous");
        next = QIcon::fromTheme("go-next");
        if (previous.isNull() || next.isNull()) {
            // It didn't work, so switch back.
            QIcon::setThemeName(originalThemeName);
        }
    }
}

const char placeholderText[] =
        "Regex filter (RE2). "
        "Case-sensitive if a capital letter exists.";

} // namespace Nav
