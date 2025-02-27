#ifndef CUSTOMTEXT_H
#define CUSTOMTEXT_H

#include <QQuickItem>

class CustomText : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
    explicit CustomText(QQuickItem *parent = nullptr);
    QString text() const { return m_text; }
    void setText(const QString &text);

signals:
    void textChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    QString m_text;
};

#endif // CUSTOMTEXT_H
