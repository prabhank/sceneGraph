#ifndef CUSTOMRECTANGLE_H
#define CUSTOMRECTANGLE_H

#include <QQuickItem>

class CustomRectangle : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    CustomRectangle(QQuickItem *parent = nullptr);
    
    QColor color() const { return m_color; }
    void setColor(const QColor &color);

signals:
    void colorChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    QColor m_color;
};

#endif // CUSTOMRECTANGLE_H
