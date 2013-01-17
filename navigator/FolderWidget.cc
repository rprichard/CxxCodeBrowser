#include "FolderWidget.h"

#include <QEvent>
#include <QFlags>
#include <QList>
#include <QMargins>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QPoint>
#include <QRect>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSize>
#include <QStyle>
#include <QStyleOptionViewItemV2>
#include <QStyleOptionViewItemV4>
#include <QWidget>
#include <algorithm>

#include "Application.h"
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
const int kIndentationPx = 16;
const int kCategoryPaddingPx = 12;
const int kItemRightMarginPx = 4;

///////////////////////////////////////////////////////////////////////////////
// FolderView

FolderWidgetView::FolderWidgetView(FileManager &fileManager, QWidget *parent) :
    QWidget(parent),
    m_fileManager(fileManager),
    m_selectedFile(NULL)
{
    setFont(Application::instance()->defaultFont());

    // For better usability, traverse the folder tree and, for every folder
    // that has only one child folder, expand the child folder.
    foreach (Folder *category, fileManager.roots())
        expandSingleChildren(category);

    updateSizeHint();
}

void FolderWidgetView::selectFile(File *file)
{
    if (m_selectedFile == file)
        return;
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
    if (changed) {
        updateSizeHint();
        update();
    }
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
            state.painter->setPen(palette().color(QPalette::Text));
            state.painter->drawText(state.origin.x(),
                                    state.origin.y() + catFM.ascent(),
                                    category->title());
        }
        state.origin.ry() += catLS;
        if (state.wantSizeHint)
            state.maxWidth = std::max(
                        state.maxWidth,
                        state.origin.x() + catFM.width(category->title()));

        traverseLayoutFolderChildren(category, state);
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
        int width =
                kIndentationPx * state.levelHasMoreSiblings.size() +
                qRound(state.itemWidthCalculator->calculate(item->title())) +
                kItemRightMarginPx;
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
        Folder *folder = item->asFolder();
        traverseLayoutFolderChildren(folder, state);
    }
}

void FolderWidgetView::traverseLayoutFolderChildren(
        Folder *folder,
        LayoutTraversalState &state) const
{
    FolderItem *lastItem =
            !folder->files().empty() ?
                static_cast<FolderItem*>(folder->files().back()) :
            !folder->folders().empty() ?
                static_cast<FolderItem*>(folder->folders().back()) : NULL;
    foreach (Folder *child, folder->folders()) {
        state.levelHasMoreSiblings.push_back(child != lastItem);
        traverseLayoutItem(child, state);
        state.levelHasMoreSiblings.pop_back();
    }
    foreach (File *child, folder->files()) {
        state.levelHasMoreSiblings.push_back(child != lastItem);
        traverseLayoutItem(child, state);
        state.levelHasMoreSiblings.pop_back();
    }
}

void FolderWidgetView::paintFolderItem(
        FolderItem *item,
        LayoutTraversalState &state) const
{
    QFlags<QStyle::StateFlag> stateFlags;
    stateFlags |= QStyle::State_Enabled;
    if (item == m_selectedFile) {
        stateFlags |= QStyle::State_Selected;
    }
    if (isActiveWindow())
        stateFlags |= QStyle::State_Active;

    {
        // Draw the row background.  (With some themes, the highlighting runs
        // from the item to the right edge, so the highlighting is done on the
        // drawControl call instead.)
        QStyleOptionViewItemV4 option;
        option.font = font();
        option.state = stateFlags;
        option.rect = QRect(0, state.origin.y(), width(), state.itemLS);
        option.showDecorationSelected = true;
        style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &option, state.painter, NULL);

        // Draw the item.  This call draws the item text, and with some themes,
        // it also draws the selection background/box.
        const int x = state.origin.x() +
                kIndentationPx * state.levelHasMoreSiblings.size();
        option.rect = QRect(x,
                            state.origin.y(),
                            width() - x, state.itemLS);
        option.text = item->title();
        option.textElideMode = Qt::ElideNone;
        option.features |= QStyleOptionViewItemV2::HasDisplay;
        style()->drawControl(QStyle::CE_ItemViewItem, &option, state.painter, NULL);
    }

    {
        // Draw branches.  In some styles (e.g. GtkStyle(Cleanlooks)), there
        // are no branch lines, but there are still arrow or plus/minus icons
        // on the folder items.
        assert(!state.levelHasMoreSiblings.empty());
        QRect primitiveRect(state.origin, QSize(kIndentationPx, state.itemLS));
        QStyleOptionViewItemV4 option;

        // For each parent, draw a sibling branch (i.e. a stylized vertical
        // line), if the parent has more siblings below the current item.
        for (size_t i = 0; i < state.levelHasMoreSiblings.size() - 1; ++i) {
            if (state.levelHasMoreSiblings[i]) {
                option.rect = primitiveRect;
                option.state = stateFlags | QStyle::State_Sibling;
                style()->drawPrimitive(
                            QStyle::PE_IndicatorBranch,
                            &option,
                            state.painter);
            }
            primitiveRect.translate(kIndentationPx, 0);
        }

        // Draw the current item's branch.  Typically, there is always a
        // vertical line from the middle of the branch rect going up.
        // State_Item enables a line from the middle of the rect going right.
        option.rect = primitiveRect;
        option.state = stateFlags | QStyle::State_Item;
        if (state.levelHasMoreSiblings.back())
            option.state |= QStyle::State_Sibling;
        if (item->isFolder()) {
            option.state |= QStyle::State_Children;
            if (m_openFolders.contains(item->asFolder()))
                option.state |= QStyle::State_Open;
        }
        style()->drawPrimitive(
                    QStyle::PE_IndicatorBranch,
                    &option,
                    state.painter);
    }
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
        ensureItemVisible(file, 0);
    }
}

void FolderWidget::ensureItemVisible(FolderItem *item, int ymargin)
{
    m_folderView->openAncestors(item);
    QRect itemBoundingRect = m_folderView->itemBoundingRect(item);
    int x = horizontalScrollBar()->value();
    ensureVisible(x, itemBoundingRect.top(),
                  0, ymargin);
    ensureVisible(x, itemBoundingRect.top() + itemBoundingRect.height(),
                  0, ymargin);
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
