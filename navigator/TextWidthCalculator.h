#ifndef NAV_TEXTWIDTHCALCULATOR_H
#define NAV_TEXTWIDTHCALCULATOR_H

#include <QFont>
#include <QFontMetrics>
#include <map>
#include <memory>

namespace Nav {

// Calculate the width of ASCII-only strings quickly.  Falls back to
// QFontMetricsF::width for strings containing non-ASCII characters.  Should
// work with both kerning (e.g. for "Wv", place v one more pixel left than
// otherwise) and subpixel widths.
class TextWidthCalculator
{
public:
    explicit TextWidthCalculator(QFontMetricsF fontMetricsF);
    int calculate(const QString &text);
    int calculate(const char *text);
    static TextWidthCalculator &getCachedTextWidthCalculator(const QFont &font);

private:
    QFontMetricsF m_fontMetricsF;
    qreal m_asciiCharWidths[128][128];
    static std::map<QFont, std::unique_ptr<TextWidthCalculator> > m_cache;
};

} // namespace Nav

#endif // NAV_TEXTWIDTHCALCULATOR_H
