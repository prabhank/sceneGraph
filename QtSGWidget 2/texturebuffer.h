#ifndef TEXTUREBUFFER_H
#define TEXTUREBUFFER_H

#include <QObject>
#include <QMutex>
#include <QSize>
#include <QByteArray>
#include <QHash>
#include <QSharedPointer>

class QQuickWindow;
class QSGTexture;

class TextureBuffer : public QObject
{
    Q_OBJECT

public:
    static TextureBuffer& instance();
    void releaseAll();
    QSGTexture* acquire(QQuickWindow* window, const QString& path);
    void releaseTexture(const QString& path);
    bool contains(const QString& path) const;

protected:
    void cleanupTexture(const QString& path);

private:
    explicit TextureBuffer(QObject *parent = nullptr);
    ~TextureBuffer();
    TextureBuffer(const TextureBuffer&) = delete;
    TextureBuffer& operator=(const TextureBuffer&) = delete;

    struct TextureInfo {
        QSGTexture* texture;
        QSharedPointer<QQuickWindow> window;
    };

    QHash<QString, TextureInfo> m_textureCache;
    mutable QMutex m_mutex;
    static constexpr int MAX_CACHE_SIZE = 50;

    void limitCacheSize();
    bool isWindowValid(const QSharedPointer<QQuickWindow>& windowPtr) const;
};

#endif // TEXTUREBUFFER_H
