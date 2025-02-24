#include "texturemanager.h"
#include <QSGTexture>
#include <QQuickWindow>
#include <QImage>
#include <QFile>
#include <QDebug>

TextureManager* TextureManager::s_instance = nullptr;

TextureManager& TextureManager::instance()
{
    static TextureManager manager;
    return manager;
}

TextureManager::TextureManager(QObject *parent)
    : QObject(parent)
{
}

TextureManager::~TextureManager()
{
    cleanup();
}

QSGTexture* TextureManager::getTexture(QQuickWindow* window, const QString& path)
{
    QMutexLocker locker(&m_mutex);
    
    // Check if texture already exists
    if (m_textures.contains(path)) {
        return m_textures[path];
    }
    
    // Load new texture
    QSGTexture* texture = loadTexture(window, path);
    if (texture) {
        m_textures[path] = texture;
    }
    
    return texture;
}

QSGTexture* TextureManager::loadTexture(QQuickWindow* window, const QString& path)
{
    if (!window) return nullptr;
    
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
    
    return window->createTextureFromImage(
        optimizedImage,
        QQuickWindow::TextureHasAlphaChannel
    );
}

void TextureManager::releaseTexture(const QString& path)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_textures.contains(path)) {
        delete m_textures[path];
        m_textures.remove(path);
    }
}

void TextureManager::cleanup()
{
    QMutexLocker locker(&m_mutex);
    
    qDeleteAll(m_textures);
    m_textures.clear();
}
