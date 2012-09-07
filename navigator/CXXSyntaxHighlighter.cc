#include "CXXSyntaxHighlighter.h"

#include <cassert>

namespace Nav {
namespace CXXSyntaxHighlighter {

#include "CXXSyntaxHighlighterDirectives.h"
#include "CXXSyntaxHighlighterKeywords.h"

// These character classification routines are inlinable and ASCII-only, which
// is good enough for a C/C++ syntax highlighter.

static inline bool isDigit(QChar ch)
{
    unsigned int ui = static_cast<unsigned>(ch.unicode());
    return ui - '0' < 10;
}

static inline bool isAlpha(QChar ch)
{
    unsigned int ui = static_cast<unsigned>(ch.unicode());
    return ui - 'a' < 26 || ui - 'A' < 26;
}

static inline bool isAlnum(QChar ch)
{
    unsigned int ui = static_cast<unsigned>(ch.unicode());
    return ui - '0' < 10 || ui - 'a' < 26 || ui - 'A' < 26;
}

QVector<Kind> highlight(const QString &content)
{
    QVector<Kind> result;
    result.resize(content.size());

    const QChar *p = content.data();
    Kind *k = result.data();
    unsigned short quoteChar = '\'';
    Kind *preprocStart;
    char text[32];
    char *pText;

#define CH(i) p[i].unicode()

START_OF_LINE:
    if (CH(0) == '#') {
        preprocStart = k;
        *k++ = KindDefault;
        p++;
        while (CH(0) == ' ' || CH(0) == '\t') {
            *k++ = KindDefault;
            p++;
        }
        pText = text;
        while (isAlpha(p[0])) {
            if (pText < text + sizeof(text) - 1)
                *pText++ = static_cast<char>(CH(0));
            *k++ = KindDefault;
            p++;
        }
        unsigned int length = pText - text;
        *pText++ = '\0';
        if (Directives::in_word_set(text, length)) {
            // Go back and highlight the directive.
            for (Kind *k2 = preprocStart; k2 != k; ++k2)
                *k2 = KindDirective;
        }

        // Handle #include <foo>: skip whitespace then jump to the QUOTED
        // state if the next character is a '<'.  Otherwise go to DEFAULT like
        // normal.
        while (CH(0) == ' ' || CH(0) == '\t') {
            *k++ = KindDefault;
            p++;
        }
        if (CH(0) == '<') {
            *k++ = KindQuoted;
            p++;
            quoteChar = '>';
            goto QUOTED;
        }

        goto DEFAULT;
    } else if (CH(0) == ' ' || CH(0) == '\t' || CH(0) == '\n') {
        *k++ = KindDefault;
        p++;
        goto START_OF_LINE;
    } else if (p[0].isNull()) {
        goto END;
    } else {
        goto DEFAULT;
    }

DEFAULT:
    if (CH(0) == '/') {
        if (CH(1) == '*') {
            *k++ = KindComment;
            *k++ = KindComment;
            p += 2;
            goto BLOCK_COMMENT;
        } else if (CH(1) == '/') {
            *k++ = KindComment;
            *k++ = KindComment;
            p += 2;
            goto LINE_COMMENT;
        }
    } else if (CH(0) == '\'' || CH(0) == '"') {
        quoteChar = CH(0);
        *k++ = KindQuoted;
        p++;
        goto QUOTED;
    } else if (isDigit(p[0]) || (CH(0) == '.' && isDigit(p[1]))) {
        goto PP_NUMBER;
    } else if (isAlpha(p[0]) || CH(0) == '_') {
        pText = text;
        goto TEXT;
    } else if (CH(0) == '\\' && CH(1) == '\n') {
        *k++ = KindDefault;
        p++;
        goto DEFAULT;
    } else if (CH(0) == '\n') {
        *k++ = KindDefault;
        p++;
        goto START_OF_LINE;
    } else if (p[0].isNull()) {
        goto END;
    } else {
        *k++ = KindDefault;
        p++;
        goto DEFAULT;
    }

BLOCK_COMMENT:
    if (CH(0) == '*' && CH(1) == '/') {
        *k++ = KindComment;
        *k++ = KindComment;
        p += 2;
        goto DEFAULT;
    } else if (p[0].isNull()) {
        goto END;
    } else {
        *k++ = KindComment;
        p++;
        goto BLOCK_COMMENT;
    }

LINE_COMMENT:
    if (CH(0) == '\n' && CH(-1) != '\\') {
        *k++ = KindComment;
        p++;
        goto START_OF_LINE;
    } else if (p[0].isNull()) {
        goto END;
    } else {
        *k++ = KindComment;
        p++;
        goto LINE_COMMENT;
    }

QUOTED:
    if (CH(0) == '\\' && CH(1) != '\0') {
        *k++ = KindQuoted;
        *k++ = KindQuoted;
        p += 2;
        goto QUOTED;
    } else if (CH(0) == quoteChar) {
        *k++ = KindQuoted;
        p++;
        goto DEFAULT;
    } else if (p[0].isNull()) {
        goto END;
    } else {
        *k++ = KindQuoted;
        p++;
        goto QUOTED;
    }

PP_NUMBER:
    // Convoluted example: 0x1._E+.12 is a single preprocessor number.
    if ((CH(0) == 'e' || CH(0) == 'E') && (CH(1) == '+' || CH(1) == '-')) {
        *k++ = KindNumber;
        *k++ = KindNumber;
        p += 2;
        goto PP_NUMBER;
    } else if (isAlnum(p[0]) || CH(0) == '_' || CH(0) == '.') {
        *k++ = KindNumber;
        p++;
        goto PP_NUMBER;
    } else {
        goto DEFAULT;
    }

TEXT:
    if (isAlnum(p[0]) || CH(0) == '_') {
        if (pText < text + sizeof(text) - 1) {
            // If we run out of room, just stop recording.  The text will
            // be too long to match a keyword anyway.
            *pText++ = static_cast<char>(CH(0));
        }
        *k++ = KindDefault;
        p++;
        goto TEXT;
    } else {
        // Check if the identifier matches a keyword, and if it does, go back
        // and mark the previous kind entries as keyword.
        unsigned int length = pText - text;
        *pText++ = '\0';
        if (Keywords::in_word_set(text, length)) {
            for (Kind *k2 = k - length; k2 != k; ++k2)
                *k2 = KindKeyword;
        }
        goto DEFAULT;
    }

#undef CH

END:
    assert(p == content.data() + content.size());
    assert(k == result.data() + result.size());

    return result;
}

} // namespace CXXSyntaxHighlighter
} // namespace Nav
