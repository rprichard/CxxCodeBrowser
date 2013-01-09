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

public:
    QString infoText() const { return m_infoText; }
    void setInfoText(const QString &infoText);
    void setInfoColors(
            const QColor &infoForeground,
            const QColor &infoBackground);
    int rightFrameWidth();
    QColor infoForeground() const { return m_infoForeground; }
    QColor infoBackground() const { return m_infoBackground; }

private:
    void keyPressEvent(QKeyEvent *event);
    void paintEvent(QPaintEvent *event);
    QString m_infoText;
    QColor m_infoForeground;
    QColor m_infoBackground;
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
            const QString &iconThemeName,
            const QString &iconResource,
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
    FindBarEdit *m_edit;
};

} // namespace Nav

#endif // NAV_FINDBAR_H
