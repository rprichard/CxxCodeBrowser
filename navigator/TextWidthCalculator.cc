#include "TextWidthCalculator.h"

#include <QChar>
#include <QFontMetricsF>
#include <QString>

namespace Nav {

const int kFirstAsciiChar = 32;
const int kLastAsciiChar = 126;

std::map<QFont, std::unique_ptr<TextWidthCalculator> >
        TextWidthCalculator::m_cache;

static inline bool isAsciiChar(int ch)
{
    return ch >= kFirstAsciiChar && ch <= kLastAsciiChar;
}

TextWidthCalculator::TextWidthCalculator(QFontMetricsF fontMetricsF) :
    m_fontMetricsF(fontMetricsF)
{
    for (int i = kFirstAsciiChar; i <= kLastAsciiChar; ++i)
        m_asciiCharWidths[0][i] = m_fontMetricsF.width(QChar(i));

    QString charPair(2, QChar());
    for (int i = kFirstAsciiChar; i <= kLastAsciiChar; ++i) {
        charPair[0] = i;
        for (int j = kFirstAsciiChar; j <= kLastAsciiChar; ++j) {
            charPair[1] = j;
            m_asciiCharWidths[i][j] =
                    m_fontMetricsF.width(charPair) -
                    m_asciiCharWidths[0][i];
        }
    }
}

qreal TextWidthCalculator::calculate(const QString &text)
{
    qreal width = 0;
    unsigned short prevChar = 0;
    for (int i = 0, iEnd = text.size(); i < iEnd; ++i) {
        unsigned short us = text[i].unicode();
        if (!isAsciiChar(us)) {
            width = m_fontMetricsF.width(text);
            break;
        }
        width += m_asciiCharWidths[prevChar][us];
        prevChar = us;
    }
    return width;
}

qreal TextWidthCalculator::calculate(const char *text)
{
    qreal width = 0;
    unsigned char prevChar = 0;
    for (const unsigned char *p =
            reinterpret_cast<const unsigned char*>(text);
            *p != '\0'; ++p) {
        if (!isAsciiChar(*p)) {
            width = m_fontMetricsF.width(text);
            break;
        }
        width += m_asciiCharWidths[prevChar][*p];
        prevChar = *p;
    }
    return width;
}

TextWidthCalculator &TextWidthCalculator::getCachedTextWidthCalculator(
        const QFont &font)
{
    if (m_cache.find(font) == m_cache.end()) {
        m_cache[font] = std::unique_ptr<TextWidthCalculator>(
                    new TextWidthCalculator(QFontMetricsF(font)));
    }
    return *m_cache[font].get();
}

} // namespace Nav
