#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QMutex>

class QSGTexture;
class QQuickWindow;

class TextureManager : public QObject
{
    Q_OBJECT

public:
    static TextureManager& instance();
    QSGTexture* getTexture(QQuickWindow* window, const QString& path);
    void cleanup();
    
protected:
    explicit TextureManager(QObject *parent = nullptr);
    ~TextureManager();

private:
    static TextureManager* s_instance;
    QHash<QString, QSGTexture*> m_textures;
    QMutex m_mutex;
    
    void releaseTexture(const QString& path);
    QSGTexture* loadTexture(QQuickWindow* window, const QString& path);
};

#endif // TEXTUREMANAGER_H
