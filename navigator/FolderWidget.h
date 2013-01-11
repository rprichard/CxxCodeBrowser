#ifndef NAV_FOLDERWIDGET_H
#define NAV_FOLDERWIDGET_H

#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSet>
#include <QSize>
#include <QWidget>

namespace Nav {

class File;
class FileManager;
class Folder;
class FolderItem;
class TextWidthCalculator;


///////////////////////////////////////////////////////////////////////////////
// FolderView

class FolderWidgetView : public QWidget
{
    Q_OBJECT
public:
    explicit FolderWidgetView(FileManager &fileManager, QWidget *parent = 0);
    void selectFile(File *file);
    File *selectedFile() { return m_selectedFile; }
    QSize sizeHint() const { return m_sizeHint; }
    QSize minimumSizeHint() const { return m_sizeHint; }
    bool ancestorsAreOpen(FolderItem *item);
    void openAncestors(FolderItem *item);
    QRect itemBoundingRect(FolderItem *item);

signals:
    void selectionChanged();
    void sizeHintChanged();

private:
    struct LayoutTraversalState {
        LayoutTraversalState();
        // inputs
        bool wantSizeHint;
        QPainter *painter;
        QRect paintRect;
        QPoint *hitTest;
        FolderItem *computeItemTop;
        // working state
        QFont itemFont;
        QFontMetrics itemFM;
        int itemLS;
        int itemAscent;
        TextWidthCalculator *itemWidthCalculator;
        int maxWidth;
        QPoint origin;
        std::vector<bool> levelHasMoreSiblings;
        // outputs
        QSize sizeHintOut;
        FolderItem *hitTestOut;
        int computeItemTopOut;
    };

    bool isRootFolder(Folder *folder);
    void expandSingleChildren(Folder *parent);
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    bool event(QEvent *event);
    void updateSizeHint();

    void traverseLayout(
            QSize *sizeHint,
            QPainter *painter,
            QPoint *hitTest,
            FolderItem **hitTestOut) const;
    void traverseLayout(LayoutTraversalState &state) const;
    void traverseLayoutItem(
            FolderItem *item,
            LayoutTraversalState &state) const;
    void traverseLayoutFolderChildren(
            Folder *folder,
            LayoutTraversalState &state) const;
    void paintFolderItem(
            FolderItem *item,
            LayoutTraversalState &state) const;

    QSize m_sizeHint;
    FolderItem *m_mousePressItem;
    FileManager &m_fileManager;
    File *m_selectedFile;
    QSet<Folder*> m_openFolders;
};


///////////////////////////////////////////////////////////////////////////////
// FolderWidget

class FolderWidget : public QScrollArea
{
    Q_OBJECT
public:
    explicit FolderWidget(FileManager &fileManager, QWidget *parent = 0);
    void selectFile(File *file);
    File *selectedFile() { return m_folderView->selectedFile(); }
    void ensureItemVisible(FolderItem *item, int ymargin=50);

signals:
    void selectionChanged();

private slots:
    void resizeFolderView();

private:
    void resizeEvent(QResizeEvent *event);

private:
    FolderWidgetView *m_folderView;
};

} // namespace Nav

#endif // NAV_FOLDERWIDGET_H
