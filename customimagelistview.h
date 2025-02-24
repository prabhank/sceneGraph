#ifndef CUSTOMIMAGELISTVIEW_H
#define CUSTOMIMAGELISTVIEW_H

#include <QQuickItem>
#include <QList>
#include <QUrl>
#include <QQmlEngine>
#include <QNetworkAccessManager>
#include <QMap>
#include <QSGGeometryNode>
#include <QImage>  // Add this include
#include "texturebuffer.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkConfiguration>
#include <QSslError>
#include <QPropertyAnimation>  // Add this include

class QSGTexture;
class QSGGeometry;

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
    Q_PROPERTY(QUrl jsonSource READ jsonSource WRITE setJsonSource NOTIFY jsonSourceChanged)

private:
    // Add the network manager to private member variables section
    QNetworkAccessManager* m_networkManager = nullptr;
    int m_count = 15;
    qreal m_itemWidth = 200;
    qreal m_itemHeight = 200;
    qreal m_spacing = 10;          // Reduced from 15 to 10
    qreal m_rowSpacing = 10;       // Reduced from 20 to 10
    bool m_useLocalImages = false;
    QString m_imagePrefix = "qrc:/images/";
    QVariantList m_imageUrls;
    QStringList m_imageTitles;
    int m_currentIndex = 0;
    QVariantList m_localImageUrls;
    QVariantList m_remoteImageUrls;
    int m_rowCount = 2;
    qreal m_contentX = 0;
    qreal m_contentY = 0;
    QStringList m_rowTitles;
    bool m_windowReady = false;
    bool m_isDestroying = false;
    bool m_isLoading = false;
    int m_itemsPerRow = 5;
    QUrl m_jsonSource;
    QMutex m_loadMutex;

    // Add new members for UI settings
    int m_titleHeight = 25; // Reduced from 30 to 25
    int m_maxRows = 4;

    void addDefaultItems();  // Add this declaration
    
    // Add helper method declaration for category width calculation
    qreal categoryContentWidth(const QString& category) const;

    // Add new members for per-category scrolling
    QMap<QString, qreal> m_categoryContentX;
    QString m_currentCategory;
    
    // Add new helper methods
    void setCategoryContentX(const QString& category, qreal x);
    qreal getCategoryContentX(const QString& category) const;
    void updateCurrentCategory();

    // Add method declaration for index validation
    void ensureValidIndex(int &index);

    // Add network manager setup method declaration
    void setupNetworkManager();

    struct CategoryDimensions {
        int rowHeight;
        int posterHeight;
        int posterWidth;
        qreal itemSpacing;
    };
    
    QMap<QString, CategoryDimensions> m_categoryDimensions;
    
    // Helper method to get dimensions for a category
    CategoryDimensions getDimensionsForCategory(const QString& category) const {
        return m_categoryDimensions.value(category, CategoryDimensions{180, 180, 280, 20});
    }

    // Add new member variables
    QPropertyAnimation* m_scrollAnimation;
    QMap<QString, QPropertyAnimation*> m_categoryAnimations;
    
    // Add helper method declarations
    void setupScrollAnimation();
    void animateScroll(const QString& category, qreal targetX);
    void stopCurrentAnimation();

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

    QUrl jsonSource() const { return m_jsonSource; }
    void setJsonSource(const QUrl &source);

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
    void jsonSourceChanged();
    void linkActivated(const QString& action, const QString& url);  // Add this signal

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
    // Add OpenGL initialization method
    void initializeGL();
    
    // Add loadAllImages declaration with other loading-related methods
    void loadAllImages();
    QString generateImageUrl(int index) const;
    QImage loadLocalImage(int index) const;
    QImage loadLocalImageFromPath(const QString &path) const;
    void loadImage(int index);
    void loadUrlImage(int index, const QUrl &url);
    void handleNetworkReply(QNetworkReply *reply, int index);
    void processLoadedImage(int index, const QImage &image);

    void debugResourceSystem() const;  // Add this line
    void tryLoadImages();
    void updateFocus();
    void navigateLeft();
    void navigateRight();
    void ensureFocus();  // Add this
    void navigateUp();
    void navigateDown();
    void ensureIndexVisible(int index);

    // Remove static texture cache as TextureBuffer handles it

    // Add new members for URL handling
    QHash<int, QNetworkReply*> m_pendingRequests;
    QHash<QUrl, QImage> m_urlImageCache;

    int getRowFromIndex(int index) const { return index / m_itemsPerRow; }
    int getColumnFromIndex(int index) const { return index % m_itemsPerRow; }

    QSGGeometryNode* createRowTitleNode(const QString &text, const QRectF &rect);

    void createFallbackTexture(int index);  // Add this declaration
    bool isReadyForTextures() const;  // Add this declaration
    void cleanupTextures();  // Add this declaration

    // Add TexturedNode structure definition before it's used
    struct TexturedNode {
        TexturedNode() : node(nullptr), texture(nullptr) {}
        ~TexturedNode() {
            delete node;
            node = nullptr;
            texture = nullptr;
        }
        QSGGeometryNode *node;
        QSGTexture *texture;
    };

    void cleanupNode(TexturedNode& node);
    QMap<int, TexturedNode> m_nodes;

    void limitTextureCacheSize(int maxTextures = 10);
    void safeReleaseTextures();
    bool ensureValidWindow() const;
    
    static constexpr int MAX_LOADED_TEXTURES = 10;

    void loadFromJson(const QUrl &source);
    void processJsonData(const QByteArray &data);
    struct ImageData {
        QString url;
        QString title;
        QString category;
        QString description;  // Add additional fields
        QString id;          // for menu item data
        QString thumbnailUrl;  // Add this field
        QMap<QString, QString> links;  // Add this to store action links (OK, info, etc)
        
        bool operator==(const ImageData& other) const {
            return url == other.url && 
                   title == other.title && 
                   category == other.category && 
                   id == other.id;
        }
    };

    QVector<ImageData> m_imageData;

    // Organize all node creation methods together in one place
    QSGGeometryNode* createTexturedRect(const QRectF &rect, QSGTexture *texture, bool isFocused = false);
   // QSGGeometryNode* createRowTitleNode(const QString &text, const QRectF &rect);
    QSGGeometryNode* createOptimizedTextNode(const QString &text, const QRectF &rect);
    void addSelectionEffects(QSGNode* container, const QRectF& rect);
    void addTitleOverlay(QSGNode* container, const QRectF& rect, const QString& title);

    // Add new method declarations
    void loadUISettings();

    // Add the declaration for calculateItemVerticalPosition
    qreal calculateItemVerticalPosition(int index);
    void handleKeyAction(Qt::Key key);  // Add this helper method

private slots:
    // Change these from declarations to actual slot definitions
    void onNetworkReplyFinished() {
        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
        if (!reply) return;

        int index = m_pendingRequests.key(reply, -1);
        if (index == -1) {
            reply->deleteLater();
            return;
        }

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            if (!data.isEmpty()) {
                QImage image;
                if (image.loadFromData(data)) {
                    m_urlImageCache.insert(reply->url(), image);
                    processLoadedImage(index, image);
                } else {
                    createFallbackTexture(index);
                }
            } else {
                createFallbackTexture(index);
            }
        } else {
            createFallbackTexture(index);
        }

        m_pendingRequests.remove(index);
        reply->deleteLater();
    }

    void onNetworkError(QNetworkReply::NetworkError code) {
        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
        if (!reply) return;

        int index = m_pendingRequests.key(reply, -1);
        if (index != -1) {
            createFallbackTexture(index);
        }
    }

    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
        if (!reply) return;

        int index = m_pendingRequests.key(reply, -1);
        if (index != -1) {
            qDebug() << "Download progress for index" << index << ":"
                     << bytesReceived << "/" << bytesTotal << "bytes";
        }
    }
};

#endif // CUSTOMIMAGELISTVIEW_H
