#ifndef NAV_CXXSYNTAXHIGHLIGHTER_H
#define NAV_CXXSYNTAXHIGHLIGHTER_H

#include <QString>
#include <memory>
#include <string>
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
    KindDirective, // preprocessor directive
    KindMax // one larger than the largest valid kind
};

typedef std::unique_ptr<Kind[]> HighlightVector;

HighlightVector highlight(const std::string &content);

} // namespace CXXSyntaxHighlighter
} // namespace Nav

#endif // NAV_CXXSYNTAXHIGHLIGHTER_H
