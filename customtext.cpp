#include "customtext.h"
#include <QQuickTextNode>
#include <QFont>

CustomText::CustomText(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void CustomText::setText(const QString &text)
{
    if (m_text != text) {
        m_text = text;
        emit textChanged();
        update();
    }
}

QSGNode *CustomText::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QQuickTextNode *node = static_cast<QQuickTextNode *>(oldNode);

    if (!node) {
        node = new QQuickTextNode(this);
    }

    // Use fixed properties for simplicity
    QFont font;
    font.setPixelSize(20);
    
    node->setFont(font);
    node->setColor(Qt::black);
    node->setText(m_text);
    
    // Position text at (0,0) relative to item
    node->setPosition(QPointF(0, 0));

    return node;
}
