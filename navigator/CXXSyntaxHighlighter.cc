#include "CXXSyntaxHighlighter.h"

#include <cassert>
#include <vector>

namespace Nav {
namespace CXXSyntaxHighlighter {

#include "CXXSyntaxHighlighterDirectives.h"
#include "CXXSyntaxHighlighterKeywords.h"

// These character classification routines are inlinable and ASCII-only, which
// is good enough for a C/C++ syntax highlighter.

static inline bool isDigit(char ch)
{
    //unsigned int ui = static_cast<unsigned>(ch.unicode());
    unsigned int ui = ch;
    return ui - '0' < 10;
}

static inline bool isAlpha(char ch)
{
    //unsigned int ui = static_cast<unsigned>(ch.unicode());
    unsigned int ui = ch;
    return ui - 'a' < 26 || ui - 'A' < 26;
}

static inline bool isAlnum(char ch)
{
    unsigned int ui = ch; // static_cast<unsigned>(ch.unicode());
    return ui - '0' < 10 || ui - 'a' < 26 || ui - 'A' < 26;
}

HighlightVector highlight(const std::string &content)
{
    auto result = HighlightVector(new Kind[content.size()]);
    const char *p = content.c_str();
    Kind *k = result.get();
    char quoteChar;
    Kind *preprocStart;
    char text[32];
    char *pText;

    const char *includeDirective =
            Directives::in_word_set("include", strlen("include"));
    const char *includeNextDirective =
            Directives::in_word_set("include_next", strlen("include_next"));
    const char *importDirective =
            Directives::in_word_set("import", strlen("import"));

#define CH(i) p[i]
#define ADVANCE(KIND) do { *k++ = KIND; p++; } while(0)

START_OF_LINE:
    if (CH(0) == '#') {
        // Handle a preprocessor directive.
        preprocStart = k;
        ADVANCE(KindDefault);
        while (CH(0) == ' ' || CH(0) == '\t') {
            ADVANCE(KindDefault);
        }
        pText = text;
        while (isAlpha(p[0]) || CH(0) == '_') {
            if (pText < text + sizeof(text) - 1)
                *pText++ = static_cast<char>(CH(0));
            ADVANCE(KindDefault);
        }
        unsigned int length = pText - text;
        *pText++ = '\0';
        const char *directive = Directives::in_word_set(text, length);
        if (directive != NULL) {
            // Go back and highlight the directive.
            for (Kind *k2 = preprocStart; k2 != k; ++k2)
                *k2 = KindDirective;

            // Handle #include <foo>.  Skip whitespace then jump to the QUOTED
            // state if the next character is a '<'.  Otherwise go to DEFAULT
            // like normal.
            if (directive == includeDirective ||
                    directive == includeNextDirective ||
                    directive == importDirective) {
                while (CH(0) == ' ' || CH(0) == '\t') {
                    ADVANCE(KindDefault);
                }
                if (CH(0) == '<') {
                    ADVANCE(KindQuoted);
                    quoteChar = '>';
                    goto QUOTED;
                }
            }
        }

        goto DEFAULT;
    } else if (CH(0) == ' ' || CH(0) == '\t' || CH(0) == '\n') {
        ADVANCE(KindDefault);
        goto START_OF_LINE;
    } else if (p[0] == '\0') {
        goto END;
    } else {
        goto DEFAULT;
    }

DEFAULT:
    if (CH(0) == '/') {
        if (CH(1) == '*') {
            ADVANCE(KindComment);
            ADVANCE(KindComment);
            goto BLOCK_COMMENT;
        } else if (CH(1) == '/') {
            ADVANCE(KindComment);
            ADVANCE(KindComment);
            goto LINE_COMMENT;
        } else {
            ADVANCE(KindDefault);
            goto DEFAULT;
        }
    } else if (CH(0) == '\'' || CH(0) == '"') {
        quoteChar = CH(0);
        ADVANCE(KindQuoted);
        goto QUOTED;
    } else if (isDigit(p[0]) || (CH(0) == '.' && isDigit(p[1]))) {
        goto PP_NUMBER;
    } else if (isAlpha(p[0]) || CH(0) == '_') {
        pText = text;
        goto TEXT;
    } else if (CH(0) == '\\' && CH(1) == '\n') {
        ADVANCE(KindDefault);
        goto DEFAULT;
    } else if (CH(0) == '\n') {
        ADVANCE(KindDefault);
        goto START_OF_LINE;
    } else if (p[0] == '\0') {
        goto END;
    } else {
        ADVANCE(KindDefault);
        goto DEFAULT;
    }

BLOCK_COMMENT:
    if (CH(0) == '*' && CH(1) == '/') {
        ADVANCE(KindComment);
        ADVANCE(KindComment);
        goto DEFAULT;
    } else if (p[0] == '\0') {
        goto END;
    } else {
        ADVANCE(KindComment);
        goto BLOCK_COMMENT;
    }

LINE_COMMENT:
    if (CH(0) == '\n' && CH(-1) != '\\') {
        // XXX: GCC/Clang allow whitespace between the backslash and newline.
        ADVANCE(KindComment);
        goto START_OF_LINE;
    } else if (p[0] == '\0') {
        goto END;
    } else {
        ADVANCE(KindComment);
        goto LINE_COMMENT;
    }

QUOTED:
    if (CH(0) == '\\' && CH(1) != '\0') {
        ADVANCE(KindQuoted);
        ADVANCE(KindQuoted);
        goto QUOTED;
    } else if (CH(0) == quoteChar) {
        ADVANCE(KindQuoted);
        goto DEFAULT;
    } else if (p[0] == '\0') {
        goto END;
    } else {
        ADVANCE(KindQuoted);
        goto QUOTED;
    }

PP_NUMBER:
    // Convoluted example: 0x1._E+.12 is a single preprocessor number.
    if ((CH(0) == 'e' || CH(0) == 'E') && (CH(1) == '+' || CH(1) == '-')) {
        ADVANCE(KindNumber);
        ADVANCE(KindNumber);
        goto PP_NUMBER;
    } else if (isAlnum(p[0]) || CH(0) == '_' || CH(0) == '.') {
        ADVANCE(KindNumber);
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
        ADVANCE(KindDefault);
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
#undef ADVANCE

END:
    assert(p == content.c_str() + content.size());
    assert(k == result.get() + content.size());

    return result;
}

} // namespace CXXSyntaxHighlighter
} // namespace Nav
