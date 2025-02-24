#include "customlistview.h"
#include <QSGNode>
#include <QSGSimpleRectNode>

CustomListView::CustomListView(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void CustomListView::setColors(const QVariantList &colors)
{
    if (m_colors != colors) {
        m_colors = colors;
        emit colorsChanged();
        update();
    }
}

void CustomListView::setItemWidth(qreal width)
{
    if (m_itemWidth != width) {
        m_itemWidth = width;
        emit itemWidthChanged();
        update();
    }
}

void CustomListView::setSpacing(qreal spacing)
{
    if (m_spacing != spacing) {
        m_spacing = spacing;
        emit spacingChanged();
        update();
    }
}

// This is painted using purely SceneGraph nodes
QSGNode *CustomListView::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *parentNode = oldNode;
    if (!parentNode) {
        parentNode = new QSGNode;
    }

    // Remove old nodes if number of items changed
    while (parentNode->childCount() > 0) {
        QSGNode *node = parentNode->firstChild();
        parentNode->removeChildNode(node);  // Changed from removeChild to removeChildNode
        delete node;
    }

    const qreal itemHeight = height();

    // Create new nodes
    for (int i = 0; i < m_colors.count() && i < 5; ++i) {
        QSGSimpleRectNode *rectNode = new QSGSimpleRectNode;
        rectNode->setColor(m_colors[i].value<QColor>());
        rectNode->setRect(i * (m_itemWidth + m_spacing), 0, m_itemWidth, itemHeight);
        parentNode->appendChildNode(rectNode);
    }

    return parentNode;
}
