#ifndef NAV_CXXSYNTAXHIGHLIGHTER_H
#define NAV_CXXSYNTAXHIGHLIGHTER_H

#include <QString>
#include <QVector>

namespace Nav {
namespace CXXSyntaxHighlighter {

enum Kind : unsigned char {
    KindDefault,
    KindComment,
    KindQuoted,
    KindNumber,
    KindKeyword,
    KindDirective // preprocessor directive
};

QVector<Kind> highlight(const QString &content);

} // namespace CXXSyntaxHighlighter
} // namespace Nav

#endif // NAV_CXXSYNTAXHIGHLIGHTER_H
