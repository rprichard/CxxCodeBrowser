#ifndef NAV_PLACEHOLDERLINEEDIT_H
#define NAV_PLACEHOLDERLINEEDIT_H

#include <QLineEdit>

namespace Nav {

// A QLineEdit with special placeholder text.  A placeholder is a greyed out
// line of text that is visible if and only if text() is empty.
//
// Qt's QLineEdit has a placeholderText property, but unfortunately, Qt does
// not display the placeholder if the widget has focus.  The query results
// window (i.e. GotoWindow) has a filter input box that always has focus.
//
class PlaceholderLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit PlaceholderLineEdit(QWidget *parent = 0);

    // Shadow QLineEdit's placeholderText property.
    void setPlaceholderText(const QString &placeholder);
    QString placeholderText() const;

protected:
    void paintEvent(QPaintEvent *event);

    QString m_placeholderText;
};

} // namespace Nav

#endif // NAV_PLACEHOLDERLINEEDIT_H
