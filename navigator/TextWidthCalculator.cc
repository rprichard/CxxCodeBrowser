#include "TextWidthCalculator.h"

#include <QTime>
#include <QDebug>
#include <QtConcurrentRun>
#include <cassert>

namespace Nav {

std::map<QFont, std::unique_ptr<TextWidthCalculator> >
        TextWidthCalculator::m_cache;

TextWidthCalculator::TextWidthCalculator(QFontMetricsF fontMetricsF) :
    m_fontMetricsF(fontMetricsF)
{
    for (int i = 32; i <= 126; ++i)
        m_asciiCharWidths[0][i] = m_fontMetricsF.width(QChar(i));

    QString charPair(2, QChar());
    for (int i = 32; i <= 126; ++i) {
        charPair[0] = i;
        for (int j = 32; j <= 126; ++j) {
            charPair[1] = j;
            m_asciiCharWidths[i][j] =
                    m_fontMetricsF.width(charPair) -
                    m_asciiCharWidths[0][i];
        }
    }
}

int TextWidthCalculator::calculate(const QString &text)
{
    qreal width = 0;
    unsigned short prevChar = 0;
    for (int i = 0, iEnd = text.size(); i < iEnd; ++i) {
        unsigned short us = text[i].unicode();
        if (us < 32 || us > 126) {
            width = m_fontMetricsF.width(text);
            break;
        }
        width += m_asciiCharWidths[prevChar][us];
        prevChar = us;
    }
    return qRound(width);
}

int TextWidthCalculator::calculate(const char *text)
{
    qreal width = 0;
    unsigned char prevChar = 0;
    for (const unsigned char *p =
            reinterpret_cast<const unsigned char*>(text);
            *p != '\0'; ++p) {
        if (*p < 32 || *p > 126) {
            width = m_fontMetricsF.width(text);
            break;
        }
        width += m_asciiCharWidths[prevChar][*p];
        prevChar = *p;
    }
    return qRound(width);
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
