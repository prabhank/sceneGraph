#if 0
#include "customtext.h"
#include <private/qquicktextnode_p.h>
#include <QFont>
#include <QSGNode>

CustomText::CustomText(QQuickItem *parent)
    : QQuickItem(parent)
    , m_layout(nullptr)
    , m_color(Qt::black)  // Initialize with default color
{
    setFlag(ItemHasContents, true);
}

CustomText::~CustomText()
{
    delete m_layout;
}

void CustomText::setText(const QString &text)
{
    if (m_text != text) {
        m_text = text;
        emit textChanged();
        update();
    }
}

void CustomText::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        emit colorChanged();
        update();
    }
}

QSGNode *CustomText::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
    QQuickTextNode *textNode = static_cast<QQuickTextNode*>(oldNode);
    if (!textNode) {
        textNode = new QQuickTextNode(this);
    }

    m_layout = new QTextLayout;
//m_layout->setFont(myFont);

    // Prepare text layout
    m_layout->setText(m_text);
    m_layout->beginLayout();
    QTextLine line = m_layout->createLine();
    if (line.isValid()) { 
        line.setLineWidth(boundingRect().width());
        line.setPosition(QPointF(0, 0)); 
        // (create more lines for multi-line text)
    }
    m_layout->endLayout();
    // Clear previous content and add new text
    textNode->deleteContent();
    textNode->addTextLayout(QPointF(0, 0), m_layout, m_color);
    return textNode;
}
#endif
