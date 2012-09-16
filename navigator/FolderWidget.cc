#include "FolderWidget.h"

#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>

#include "File.h"
#include "FileManager.h"
#include "Folder.h"
#include "FolderItem.h"
#include "MainWindow.h"
#include "Misc.h"

namespace Nav {

const QMargins kFolderViewMargins(3, 3, 3, 3);
const int kExtraItemTopPx = 1;
const int kExtraItemBottomPx = 1;
const int kBranchSizePx = 16;
const int kBranchMarginPx = 18;
const int kCategoryPaddingPx = 12;

///////////////////////////////////////////////////////////////////////////////
// FolderView

FolderWidgetView::FolderWidgetView(FileManager &fileManager, QWidget *parent) :
    QWidget(parent),
    m_fileManager(fileManager),
    m_selectedFile(NULL)
{
    QFont f = font();
    f.setPointSize(9);
    setFont(f);

    // For better usability, traverse the folder tree and, for every folder
    // that has only one child folder, expand the child folder.
    foreach (Folder *category, fileManager.roots())
        expandSingleChildren(category);

    updateSizeHint();
}

void FolderWidgetView::selectFile(File *file)
{
    m_selectedFile = file;
    update();
}

bool FolderWidgetView::ancestorsAreOpen(FolderItem *item)
{
    Folder *parent = item->parent();
    while (!isRootFolder(parent)) {
        if (!m_openFolders.contains(parent))
            return false;
        parent = parent->parent();
    }
    return true;
}

void FolderWidgetView::openAncestors(FolderItem *item)
{
    Folder *parent = item->parent();
    bool changed = false;
    while (!isRootFolder(parent)) {
        if (!m_openFolders.contains(parent)) {
            m_openFolders.insert(parent);
            changed = true;
        }
        parent = parent->parent();
    }
    if (changed)
        updateSizeHint();
}

QRect FolderWidgetView::itemBoundingRect(FolderItem *item)
{
    LayoutTraversalState state;
    state.computeItemTop = item;
    traverseLayout(state);
    QRect result;
    if (state.computeItemTopOut != -1) {
        result.setTop(state.computeItemTopOut);
        result.setHeight(state.itemLS);
        result.setLeft(0);
        result.setWidth(width());
    }
    return result;
}

FolderWidgetView::LayoutTraversalState::LayoutTraversalState() :
    wantSizeHint(false),
    painter(NULL),
    hitTest(NULL),
    computeItemTop(NULL),
    itemFM(itemFont),
    itemLS(0),
    itemAscent(0),
    itemWidthCalculator(NULL),
    maxWidth(0),
    hitTestOut(NULL),
    computeItemTopOut(-1)
{
}

bool FolderWidgetView::isRootFolder(Folder *folder)
{
    return folder->parent() == NULL;
}

void FolderWidgetView::expandSingleChildren(Folder *parent)
{
    if (parent->folders().size() == 1 &&
            parent->files().size() == 0)
        m_openFolders.insert(parent->folders()[0]);
    foreach (Folder *folder, parent->folders())
        expandSingleChildren(folder);
}

void FolderWidgetView::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    LayoutTraversalState state;
    state.painter = &painter;
    state.paintRect = event->rect();
    traverseLayout(state);
}

void FolderWidgetView::mousePressEvent(QMouseEvent *event)
{
    m_mousePressItem = NULL;
    QPoint pt = event->pos();
    traverseLayout(NULL, NULL, &pt, &m_mousePressItem);
    if (m_mousePressItem != NULL && !m_mousePressItem->isFolder()) {
        // Select this file.
        m_selectedFile = m_mousePressItem->asFile();
        update();
        emit selectionChanged();
    }
}

void FolderWidgetView::mouseMoveEvent(QMouseEvent *event)
{
    FolderItem *item = NULL;
    QPoint pt = event->pos();
    traverseLayout(NULL, NULL, &pt, &item);
    if (m_mousePressItem != NULL && item != NULL &&
            !m_mousePressItem->isFolder() && !item->isFolder()) {
        // Select this file.
        m_selectedFile = item->asFile();
        update();
        emit selectionChanged();
    }
}

void FolderWidgetView::mouseReleaseEvent(QMouseEvent *event)
{
    FolderItem *item = NULL;
    QPoint pt = event->pos();
    traverseLayout(NULL, NULL, &pt, &item);
    if (item != NULL && item == m_mousePressItem) {
        // This item was clicked.
        if (item->isFolder()) {
            // Toggle the visibility of the folder.
            if (m_openFolders.contains(item->asFolder()))
                m_openFolders.remove(item->asFolder());
            else
                m_openFolders.insert(item->asFolder());
            update();
            updateSizeHint();
        }
    }
}

bool FolderWidgetView::event(QEvent *event)
{
    // TODO: tooltip handling
    return QWidget::event(event);
}

void FolderWidgetView::updateSizeHint()
{
    LayoutTraversalState state;
    state.wantSizeHint = true;
    traverseLayout(state);
    if (m_sizeHint != state.sizeHintOut) {
        m_sizeHint = state.sizeHintOut;
        emit sizeHintChanged();
    }
}

void FolderWidgetView::traverseLayout(
        QSize *sizeHint,
        QPainter *painter,
        QPoint *hitTest,
        FolderItem **hitTestOut) const
{
    assert((hitTest == NULL) == (hitTestOut == NULL));
    LayoutTraversalState state;
    state.wantSizeHint = (sizeHint != NULL);
    state.painter = painter;
    state.hitTest = hitTest;
    traverseLayout(state);
    if (hitTestOut != NULL)
        *hitTestOut = state.hitTestOut;
    if (sizeHint != NULL)
        *sizeHint = state.sizeHintOut;
}

void FolderWidgetView::traverseLayout(LayoutTraversalState &state) const
{
    QFont catFont = font();
    catFont.setBold(true);
    const QFontMetrics catFM(catFont);
    const int catLS = effectiveLineSpacing(catFM);

    state.itemFont = font();
    state.itemFM = QFontMetrics(state.itemFont);
    state.itemLS = effectiveLineSpacing(state.itemFM) +
            kExtraItemTopPx + kExtraItemBottomPx;
    state.itemAscent = state.itemFM.ascent() + kExtraItemTopPx;
    state.itemWidthCalculator =
            &TextWidthCalculator::getCachedTextWidthCalculator(state.itemFont);

    state.origin = QPoint(kFolderViewMargins.left(),
                          kFolderViewMargins.top());
    QList<Folder*> categories = m_fileManager.roots();

    bool firstCategory = true;
    foreach (Folder *category, categories) {
        // Inter-category padding.
        if (!firstCategory)
            state.origin.ry() += kCategoryPaddingPx;
        firstCategory = false;

        // Handle the category title.
        if (state.painter != NULL) {
            state.painter->setFont(catFont);
            state.painter->drawText(state.origin.x(),
                                    state.origin.y() + catFM.ascent(),
                                    category->title());
        }
        state.origin.ry() += catLS;
        if (state.wantSizeHint)
            state.maxWidth = std::max(
                        state.maxWidth,
                        state.origin.x() + catFM.width(category->title()));

        // Traverse the children.
        foreach (FolderItem *item, category->folders())
            traverseLayoutItem(item, state);
        foreach (FolderItem *item, category->files())
            traverseLayoutItem(item, state);
    }
    state.origin.ry() += kFolderViewMargins.bottom();
    state.sizeHintOut = QSize(state.maxWidth + kFolderViewMargins.right(),
                              state.origin.y());
}

void FolderWidgetView::traverseLayoutItem(
        FolderItem *item,
        LayoutTraversalState &state) const
{
    const int top = state.origin.y();
    const int height = state.itemLS;
    if (state.wantSizeHint) {
        int width = kBranchMarginPx +
                    state.itemWidthCalculator->calculate(item->title());
        state.maxWidth = std::max(state.maxWidth, state.origin.x() + width);
    }
    if (state.painter != NULL &&
            top < state.paintRect.bottom() + 10 &&
            top + height > state.paintRect.top() - 10) {
        paintFolderItem(item, state);
    }
    if (state.hitTest != NULL) {
        int y = state.hitTest->y();
        if (y >= state.origin.y() && y < state.origin.y() + height)
            state.hitTestOut = item;
    }
    state.origin.ry() += height;

    if (state.computeItemTop == item)
        state.computeItemTopOut = top;

    // Children.
    if (item->isFolder() && m_openFolders.contains(item->asFolder())) {
        const int indentPx = 12;
        state.origin.rx() += indentPx;
        foreach (FolderItem *child, item->asFolder()->folders())
            traverseLayoutItem(child, state);
        foreach (FolderItem *child, item->asFolder()->files())
            traverseLayoutItem(child, state);
        state.origin.rx() -= indentPx;
    }
}

void FolderWidgetView::paintFolderItem(
        FolderItem *item,
        LayoutTraversalState &state) const
{
    state.painter->setFont(state.itemFont);
    if (item->isFolder()) {
        QStyleOption option;
        option.state |= QStyle::State_Children;
        if (m_openFolders.contains(item->asFolder()))
            option.state |= QStyle::State_Open;
        option.rect = QRect(state.origin.x(), state.origin.y(), kBranchSizePx, state.itemLS);
        style()->drawPrimitive(
                    QStyle::PE_IndicatorBranch,
                    &option,
                    state.painter);
    }
    if (item == m_selectedFile) {
        // Draw the selection background.
        QStyleOptionViewItemV4 option;
        option.rect = QRect(0, state.origin.y(), width(), state.itemLS);
        option.state |= QStyle::State_Selected;
        style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &option, state.painter, this);

        // Use the selection text color.
        state.painter->setPen(palette().color(QPalette::HighlightedText));
    } else {
        // Use the default text color.
        state.painter->setPen(palette().color(QPalette::Text));
    }
    state.painter->drawText(state.origin.x() + kBranchMarginPx,
                            state.origin.y() + state.itemAscent,
                            item->title());
}


///////////////////////////////////////////////////////////////////////////////
// FolderWidget

FolderWidget::FolderWidget(FileManager &fileManager, QWidget *parent) :
    QScrollArea(parent),
    m_folderView(new FolderWidgetView(fileManager))
{
    setWidgetResizable(false);
    setWidget(m_folderView);
    connect(m_folderView,
            SIGNAL(sizeHintChanged()),
            SLOT(resizeFolderView()));
    connect(m_folderView,
            SIGNAL(selectionChanged()),
            SIGNAL(selectionChanged()));
    setBackgroundRole(QPalette::Base);
    setFocusPolicy(Qt::NoFocus);
}

void FolderWidget::selectFile(File *file)
{
    m_folderView->selectFile(file);
    if (m_folderView->ancestorsAreOpen(file)) {
        ensureItemVisible(file);
    }
}

void FolderWidget::ensureItemVisible(FolderItem *item)
{
    m_folderView->openAncestors(item);
    QRect itemBoundingRect = m_folderView->itemBoundingRect(item);
    int x = horizontalScrollBar()->value();
    ensureVisible(x, itemBoundingRect.top());
}

void FolderWidget::resizeFolderView()
{
    QSize size = m_folderView->sizeHint();
    size = size.expandedTo(viewport()->size());
    m_folderView->resize(size);
}

void FolderWidget::resizeEvent(QResizeEvent *event)
{
    resizeFolderView();
    return QScrollArea::resizeEvent(event);
}

} // namespace Nav
