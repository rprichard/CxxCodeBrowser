#ifndef NAV_CXXSYNTAXHIGHLIGHTER_H
#define NAV_CXXSYNTAXHIGHLIGHTER_H

#include <QString>
#include <vector>

namespace Nav {

// The CXXSyntaxHighlighter scans a file's content and classifies each
// character as one of several syntax kinds.  Each syntax kind can then be
// painted using a different color.
namespace CXXSyntaxHighlighter {

enum Kind : unsigned char {
    KindDefault,
    KindComment,
    KindQuoted,
    KindNumber,
    KindKeyword,
    KindDirective // preprocessor directive
};

std::vector<Kind> highlight(const QString &content);

} // namespace CXXSyntaxHighlighter
} // namespace Nav

#endif // NAV_CXXSYNTAXHIGHLIGHTER_H
