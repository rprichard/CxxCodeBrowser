#ifndef NAV_FINDBAR_H
#define NAV_FINDBAR_H

#include <QFrame>
#include <QString>
#include <QStyleOptionFrameV2>

#include "PlaceholderLineEdit.h"
#include "Regex.h"

class QLabel;
class QToolButton;

namespace Nav {


///////////////////////////////////////////////////////////////////////////////
// FindBarEdit

class FindBarEdit : public PlaceholderLineEdit {
    Q_OBJECT
public:
    FindBarEdit(QWidget *parent = 0);

signals:
    void shiftReturnPressed();
    void escapePressed();

private:
    void keyPressEvent(QKeyEvent *event);
};


///////////////////////////////////////////////////////////////////////////////
// FindBarEditFrame

class FindBarEditFrame : public QWidget {
    Q_OBJECT
public:
    FindBarEditFrame(QWidget *parent = 0);
    int frameWidth();
    void initStyleOption(QStyleOptionFrameV2 &option);
private:
    void paintEvent(QPaintEvent *event);
};


///////////////////////////////////////////////////////////////////////////////
// FindBar

class FindBar : public QFrame {
    Q_OBJECT
public:
    explicit FindBar(QWidget *parent = 0);
    const Regex &regex();

private:
    QToolButton *makeButton(
            const QString &icon,
            const QString &name,
            const QString &tooltip,
            const char *signal);

signals:
    void previous();
    void next();
    void closeBar();
    void regexChanged();

public slots:
    void setMatchInfo(int index, int count);
    void selectAll();

private slots:
    void onEditTextChanged();

private:
    FindBarEdit *m_edit;
    QLabel *m_infoLabel;
    Regex m_regex;
};

} // namespace Nav

#endif // NAV_FINDBAR_H
