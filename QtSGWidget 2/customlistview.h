#ifndef CUSTOMLISTVIEW_H
#define CUSTOMLISTVIEW_H

#include <QQuickItem>
#include <QList>
#include <QColor>

class CustomListView : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QVariantList colors READ colors WRITE setColors NOTIFY colorsChanged)
    Q_PROPERTY(qreal itemWidth READ itemWidth WRITE setItemWidth NOTIFY itemWidthChanged)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)

public:
    CustomListView(QQuickItem *parent = nullptr);

    QVariantList colors() const { return m_colors; }
    void setColors(const QVariantList &colors);

    qreal itemWidth() const { return m_itemWidth; }
    void setItemWidth(qreal width);

    qreal spacing() const { return m_spacing; }
    void setSpacing(qreal spacing);

signals:
    void colorsChanged();
    void itemWidthChanged();
    void spacingChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    QVariantList m_colors;
    qreal m_itemWidth = 200;
    qreal m_spacing = 15;
};

#endif // CUSTOMLISTVIEW_H
