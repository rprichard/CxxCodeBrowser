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
    QSize sizeHint() const;
    QSize minimumSizeHint() const { return sizeHint(); }
    FindBarEdit *findBarEdit() { return m_edit; }
    QLabel *infoLabel() { return m_infoLabel; }
private:
    void initStyleOption(QStyleOptionFrameV2 &option) const;
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    bool event(QEvent *event);
    void layoutChildren();

    FindBarEdit *m_edit;
    QLabel *m_infoLabel;
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
    Regex m_regex;
    FindBarEditFrame *m_editFrame;
};

} // namespace Nav

#endif // NAV_FINDBAR_H
