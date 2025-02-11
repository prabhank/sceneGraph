#include "texturebuffer.h"
#include <QQuickWindow>
#include <QSGTexture>
#include <QImage>
#include <QFile>
#include <QDebug>

TextureBuffer& TextureBuffer::instance()
{
    static TextureBuffer instance;
    return instance;
}

TextureBuffer::TextureBuffer(QObject *parent)
    : QObject(parent)
{
}

TextureBuffer::~TextureBuffer()
{
    releaseAll();
}

QSGTexture* TextureBuffer::acquire(QQuickWindow* window, const QString& path)
{
    if (!window) return nullptr;

    QMutexLocker locker(&m_mutex);

    // Check if texture exists and is valid
    if (m_textureCache.contains(path)) {
        TextureInfo& info = m_textureCache[path];
        if (isWindowValid(info.window) && info.texture) {
            return info.texture;
        } else {
            cleanupTexture(path);
        }
    }

    // Load new texture
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open image file:" << path;
        return nullptr;
    }

    QImage image;
    if (!image.loadFromData(file.readAll())) {
        qWarning() << "Failed to load image data from:" << path;
        return nullptr;
    }

    // Convert to optimal format for textures
    QImage optimizedImage = image.convertToFormat(QImage::Format_RGBA8888);

    // Create texture
    QSGTexture* texture = window->createTextureFromImage(
        optimizedImage,
        QQuickWindow::TextureHasAlphaChannel
    );

    if (texture) {
        limitCacheSize();
        TextureInfo info;
        info.texture = texture;
        info.window = QSharedPointer<QQuickWindow>(window, [](QQuickWindow*) {});  // Non-owning shared pointer
        m_textureCache.insert(path, info);
    }

    return texture;
}

void TextureBuffer::releaseAll()
{
    QMutexLocker locker(&m_mutex);
    QStringList keys = m_textureCache.keys();
    for (const QString& path : keys) {
        cleanupTexture(path);
    }
    m_textureCache.clear();
}

void TextureBuffer::cleanupTexture(const QString& path)
{
    if (m_textureCache.contains(path)) {
        TextureInfo& info = m_textureCache[path];
        if (info.texture) {
            delete info.texture;
        }
        m_textureCache.remove(path);
    }
}

void TextureBuffer::limitCacheSize()
{
    while (m_textureCache.size() >= MAX_CACHE_SIZE) {
        QString oldestKey = m_textureCache.keys().first();
        cleanupTexture(oldestKey);
    }
}

bool TextureBuffer::isWindowValid(const QSharedPointer<QQuickWindow>& windowPtr) const
{
    return !windowPtr.isNull() && windowPtr.data() != nullptr;
}

bool TextureBuffer::contains(const QString& path) const
{
    QMutexLocker locker(&m_mutex);
    return m_textureCache.contains(path);
}

void TextureBuffer::releaseTexture(const QString& path)
{
    QMutexLocker locker(&m_mutex);
    cleanupTexture(path);
}
