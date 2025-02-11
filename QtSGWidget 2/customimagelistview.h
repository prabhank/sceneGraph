#ifndef CUSTOMIMAGELISTVIEW_H
#define CUSTOMIMAGELISTVIEW_H

#include <QQuickItem>
#include <QList>
#include <QUrl>
#include <QQmlEngine>
#include <QNetworkAccessManager>
#include <QMap>
#include "texturebuffer.h"

class QSGTexture;
class QSGGeometryNode;
class QSGGeometry;  // Add this forward declaration

class CustomImageListView : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    Q_PROPERTY(qreal itemWidth READ itemWidth WRITE setItemWidth NOTIFY itemWidthChanged)
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(bool useLocalImages READ useLocalImages WRITE setUseLocalImages NOTIFY useLocalImagesChanged)
    Q_PROPERTY(QString imagePrefix READ imagePrefix WRITE setImagePrefix NOTIFY imagePrefixChanged)
    Q_PROPERTY(QVariantList imageUrls READ imageUrls WRITE setImageUrls NOTIFY imageUrlsChanged)
    Q_PROPERTY(QStringList imageTitles READ imageTitles WRITE setImageTitles NOTIFY imageTitlesChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QVariantList localImageUrls READ localImageUrls WRITE setLocalImageUrls NOTIFY localImageUrlsChanged)
    Q_PROPERTY(QVariantList remoteImageUrls READ remoteImageUrls WRITE setRemoteImageUrls NOTIFY remoteImageUrlsChanged)
    Q_PROPERTY(int rowCount READ rowCount WRITE setRowCount NOTIFY rowCountChanged)
    Q_PROPERTY(qreal contentX READ contentX WRITE setContentX NOTIFY contentXChanged)
    Q_PROPERTY(qreal contentY READ contentY WRITE setContentY NOTIFY contentYChanged)
    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY contentWidthChanged)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(qreal itemHeight READ itemHeight WRITE setItemHeight NOTIFY itemHeightChanged)
    Q_PROPERTY(qreal rowSpacing READ rowSpacing WRITE setRowSpacing NOTIFY rowSpacingChanged)
    Q_PROPERTY(QStringList rowTitles READ rowTitles WRITE setRowTitles NOTIFY rowTitlesChanged)

public:
    CustomImageListView(QQuickItem *parent = nullptr);
    ~CustomImageListView();

    int count() const { return m_count; }
    void setCount(int count);

    qreal itemWidth() const { return m_itemWidth; }
    void setItemWidth(qreal width);

    qreal spacing() const { return m_spacing; }
    void setSpacing(qreal spacing);

    bool useLocalImages() const { return m_useLocalImages; }
    void setUseLocalImages(bool local);
    
    QString imagePrefix() const { return m_imagePrefix; }
    void setImagePrefix(const QString &prefix);

    QVariantList imageUrls() const { return m_imageUrls; }
    void setImageUrls(const QVariantList &urls);

    QStringList imageTitles() const { return m_imageTitles; }
    void setImageTitles(const QStringList &titles);

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    QVariantList localImageUrls() const { return m_localImageUrls; }
    void setLocalImageUrls(const QVariantList &urls);
    
    QVariantList remoteImageUrls() const { return m_remoteImageUrls; }
    void setRemoteImageUrls(const QVariantList &urls);
    
    int rowCount() const { return m_rowCount; }
    void setRowCount(int count);

    qreal contentX() const { return m_contentX; }
    void setContentX(qreal x);
    
    qreal contentY() const { return m_contentY; }
    void setContentY(qreal y);
    
    qreal contentWidth() const;
    qreal contentHeight() const;

    qreal itemHeight() const { return m_itemHeight; }
    void setItemHeight(qreal height);
    
    qreal rowSpacing() const { return m_rowSpacing; }
    void setRowSpacing(qreal spacing);
    
    QStringList rowTitles() const { return m_rowTitles; }
    void setRowTitles(const QStringList &titles);

signals:
    void countChanged();
    void itemWidthChanged();
    void spacingChanged();
    void useLocalImagesChanged();
    void imagePrefixChanged();
    void imageUrlsChanged();
    void imageTitlesChanged();
    void currentIndexChanged();
    void localImageUrlsChanged();
    void remoteImageUrlsChanged();
    void rowCountChanged();
    void contentXChanged();
    void contentYChanged();
    void contentWidthChanged();
    void contentHeightChanged();
    void itemHeightChanged();
    void rowSpacingChanged();
    void rowTitlesChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void componentComplete() override;
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;  // Add this
    void wheelEvent(QWheelEvent *event) override;

private:
    QString generateImageUrl(int index) const;
    void loadImage(int index);
    void loadAllImages();

    struct TexturedNode {
        TexturedNode() : node(nullptr), texture(nullptr), index(-1) {}
        ~TexturedNode() {
            delete node;
            // Don't delete texture here, TextureBuffer manages it
            node = nullptr;
            texture = nullptr;
        }
        QSGGeometryNode *node;
        QSGTexture *texture;
        int index;  // Add this member
    };

    QSGGeometryNode* createTexturedRect(const QRectF &rect, QSGTexture *texture);
    QSGGeometryNode* createTextNode(const QString &text, const QRectF &rect);
    QSGGeometryNode* createOptimizedTextNode(const QString &text, const QRectF &rect);  // Add this for optimized text node

    QMap<int, TexturedNode> m_nodes;

    int m_count = 15;  // 3 rows x 5 images
    qreal m_itemWidth = 200;
    qreal m_spacing = 15;
    QNetworkAccessManager *m_networkManager;
    QMap<int, QImage> m_loadedImages;
    bool m_useLocalImages = false;
    QString m_imagePrefix = "qrc:/images/";
    QVariantList m_imageUrls;
    QStringList m_imageTitles;
    QImage loadLocalImage(int index) const;
    void debugResourceSystem() const;  // Add this line
    bool m_windowReady = false;
    void tryLoadImages();
    int m_currentIndex = 0;
    void updateFocus();
    void navigateLeft();
    void navigateRight();
    void ensureFocus();  // Add this
    void navigateUp();
    void navigateDown();
    void ensureIndexVisible(int index);
    qreal m_contentX = 0;
    qreal m_contentY = 0;

    // Remove static texture cache as TextureBuffer handles it

    // Add new members for URL handling
    void loadUrlImage(int index, const QUrl &url);
    void handleNetworkReply(QNetworkReply *reply, int index);
    QHash<int, QNetworkReply*> m_pendingRequests;
    QHash<QUrl, QImage> m_urlImageCache;
    void processLoadedImage(int index, const QImage &image);  // Add this declaration

    QVariantList m_localImageUrls;
    QVariantList m_remoteImageUrls;
    int m_rowCount = 2;
    int getRowFromIndex(int index) const { return index / m_itemsPerRow; }
    int getColumnFromIndex(int index) const { return index % m_itemsPerRow; }
    int m_itemsPerRow = 5;  // Add this member

    qreal m_itemHeight = 200;
    qreal m_rowSpacing = 40;
    QStringList m_rowTitles;
    QSGGeometryNode* createRowTitleNode(const QString &text, const QRectF &rect);

    void createFallbackTexture(int index);  // Add this declaration
    bool isReadyForTextures() const;  // Add this declaration
    void cleanupTextures();  // Add this declaration

    void cleanupNode(TexturedNode& node);
    void limitTextureCacheSize(int maxTextures = 10);
    void safeReleaseTextures();
    bool ensureValidWindow() const;
    
    QMutex m_loadMutex;
    bool m_isDestroying = false;
    
    static constexpr int MAX_LOADED_TEXTURES = 10;
    bool m_isLoading = false;
};

#endif // CUSTOMIMAGELISTVIEW_H
