#ifndef NAV_TEXTWIDTHCALCULATOR_H
#define NAV_TEXTWIDTHCALCULATOR_H

#include <QFont>
#include <QFontMetricsF>
#include <QString>
#include <map>
#include <memory>

namespace Nav {

// Calculate the width of ASCII-only strings quickly.  Falls back to
// QFontMetricsF::width for strings containing non-ASCII characters.  Should
// work with both kerning (e.g. for "Wv", place v one more pixel left than
// otherwise) and subpixel widths.
//
// The qreal values are typically (always?) multiples of a
// negative-power-of-two no smaller than 1/64.  Therefore, it is feasible to
// track character positions without rounding errors.
class TextWidthCalculator
{
public:
    explicit TextWidthCalculator(QFontMetricsF fontMetricsF);
    qreal calculate(const QString &text);
    qreal calculate(const char *text);
    static TextWidthCalculator &getCachedTextWidthCalculator(const QFont &font);

private:
    QFontMetricsF m_fontMetricsF;
    qreal m_asciiCharWidths[128][128];
    static std::map<QFont, std::unique_ptr<TextWidthCalculator> > m_cache;
};

} // namespace Nav

#endif // NAV_TEXTWIDTHCALCULATOR_H
