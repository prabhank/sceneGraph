#if 0
#ifndef CUSTOMTEXT_H
#define CUSTOMTEXT_H

#include <QQuickItem>
#include <QTextLayout>
#include <QColor>

class CustomText : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit CustomText(QQuickItem *parent = nullptr);
    ~CustomText();
    QString text() const { return m_text; }
    void setText(const QString &text);
    QColor color() const { return m_color; }
    void setColor(const QColor &color);

signals:
    void textChanged();
    void colorChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    QString m_text;
    QTextLayout *m_layout;
    QColor m_color;
};

#endif // CUSTOMTEXT_H
#endif