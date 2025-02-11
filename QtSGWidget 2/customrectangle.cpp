#include "customrectangle.h"
#include <QSGSimpleRectNode>

CustomRectangle::CustomRectangle(QQuickItem *parent)
    : QQuickItem(parent)
    , m_color(Qt::red)
{
    setFlag(ItemHasContents, true);
}

void CustomRectangle::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        emit colorChanged();
        update();
    }
}

QSGNode *CustomRectangle::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGSimpleRectNode *node = static_cast<QSGSimpleRectNode *>(oldNode);

    if (!node) {
        node = new QSGSimpleRectNode(boundingRect(), m_color);
    } else {
        node->setRect(boundingRect());
        node->setColor(m_color);
    }

    return node;
}
