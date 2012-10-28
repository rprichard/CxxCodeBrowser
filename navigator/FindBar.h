#ifndef NAV_FINDBAR_H
#define NAV_FINDBAR_H

#include <QFrame>
#include <QLineEdit>
#include <QString>
#include <QStyleOptionFrameV2>

#include "Regex.h"

class QLabel;
class QToolButton;

namespace Nav {


///////////////////////////////////////////////////////////////////////////////
// FindBarEdit

class FindBarEdit : public QLineEdit {
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
    Regex regex();

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
    void textChanged();

public slots:
    void setMatchInfo(int index, int count);
    void selectAll();

private:
    FindBarEdit *m_edit;
    QLabel *m_infoLabel;
};

} // namespace Nav

#endif // NAV_FINDBAR_H
