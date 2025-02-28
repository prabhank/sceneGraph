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
#include <QSGSimpleTextureNode>
#include <QSGTextureMaterial>
#include <QSGOpaqueTextureMaterial>
#include <QSGFlatColorMaterial>
#include "texturebuffer.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkConfiguration>
#include <QSslError>
#include <QPropertyAnimation>  // Add this include
#include <QSet>  // Add this include
#include <QRunnable>  // Add this line to include QRunnable
#include <QPointer>



class QSGTexture;
class QSGGeometry;

// Add this better SafeNodeDeleter definition
class SafeNodeDeleter : public QRunnable {
    public:
        explicit SafeNodeDeleter(QSGNode* node) : m_node(node) {}
        
        void run() override {
            if (m_node) {
                delete m_node;
            }
        }
        
    private:
        QSGNode* m_node;
};

// Add this new batch deleter for multiple nodes
class SafeNodeBatchDeleter : public QRunnable {
    public:
        explicit SafeNodeBatchDeleter(const QList<QSGNode*>& nodes) : m_nodes(nodes) {}
        
        void run() override {
            for (QSGNode* node : m_nodes) {
                if (node) {
                    delete node;
                }
            }
            m_nodes.clear();
        }
        
    private:
        QList<QSGNode*> m_nodes;
};

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
    Q_PROPERTY(qreal startPositionX READ startPositionX WRITE setStartPositionX NOTIFY startPositionXChanged)
    Q_PROPERTY(int nodeCount READ nodeCount CONSTANT)  // Simplified read-only property
    Q_PROPERTY(int textureCount READ textureCount CONSTANT)  // Simplified read-only property
    Q_PROPERTY(bool enableNodeMetrics READ enableNodeMetrics WRITE setEnableNodeMetrics NOTIFY enableNodeMetricsChanged)
    Q_PROPERTY(bool enableTextureMetrics READ enableTextureMetrics WRITE setEnableTextureMetrics NOTIFY enableTextureMetricsChanged)

private:
    // Move ImageData struct definition to the top of the private section
    struct ImageData {
        QString url;
        QString title;
        QString category;
        QString description;
        QString id;
        QString thumbnailUrl;
        QMap<QString, QString> links;
        
        bool operator==(const ImageData& other) const {
            return url == other.url && 
                   title == other.title && 
                   category == other.category && 
                   id == other.id;
        }
    };

    // Now we can use ImageData in member variables
    QNetworkAccessManager* m_networkManager = nullptr;
    QVector<ImageData> m_imageData;
    qreal m_startPositionX = 0;  // Add this line for the start position
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
    bool m_enableNodeMetrics = false;
    bool m_enableTextureMetrics = false;

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

    // Add member to store complete JSON document
    QJsonObject m_parsedJson;  // Add this line

    // Add flag to track destruction state
    bool m_isBeingDestroyed = false;

    // Add method for safe cleanup
    void safeCleanup();

    // Only keep track of node count
    int m_nodeCount = 0;
    int m_totalNodeCount = 0;  // Add this to store total node count
    QAtomicInt m_textureCount = 0;  // Add this to store texture count

    // Add helper method to count nodes recursively
    int countNodes(QSGNode *root) {
        if (!root)
            return 0;

        int total = 1; // count the current node
        for (QSGNode *child = root->firstChild(); child; child = child->nextSibling()) {
            total += countNodes(child);
        }
        return total;
    }

    // Modified version using QSet instead of std::unordered_set
    void collectTextures(QSGNode *node, QSet<QSGTexture *> &textures) {
        if (!node) return;

        // Check geometry node materials
        if (node->type() == QSGNode::GeometryNodeType) {
            QSGGeometryNode *geometryNode = static_cast<QSGGeometryNode*>(node);
            QSGMaterial *mat = geometryNode->activeMaterial();
            if (mat) {
                // QSGTextureMaterial
                QSGTextureMaterial *texMat = dynamic_cast<QSGTextureMaterial*>(mat);
                if (texMat && texMat->texture()) {
                    textures.insert(texMat->texture());
                }
                // QSGOpaqueTextureMaterial
                QSGOpaqueTextureMaterial *opaqueTexMat = dynamic_cast<QSGOpaqueTextureMaterial*>(mat);
                if (opaqueTexMat && opaqueTexMat->texture()) {
                    textures.insert(opaqueTexMat->texture());
                }
            }
        }

        // Check QSGSimpleTextureNode
        QSGSimpleTextureNode *simpleTex = dynamic_cast<QSGSimpleTextureNode*>(node);
        if (simpleTex && simpleTex->texture()) {
            textures.insert(simpleTex->texture());
        }

        // Recurse through children
        for (QSGNode *child = node->firstChild(); child; child = child->nextSibling()) {
            collectTextures(child, textures);
        }
    }

    // Modified version using QSet instead of std::unordered_set
    int countTotalTextures(QSGNode *root) {
        QSet<QSGTexture *> textures;
        collectTextures(root, textures);
        return textures.size();
    }

    // Add these new methods
    QVector<int> getVisibleIndices();
    void handleContentPositionChange();

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

    qreal startPositionX() const { return m_startPositionX; }
    void setStartPositionX(qreal x);

    // Update accessors to get real-time counts
    int nodeCount() const { return m_totalNodeCount; }
    int textureCount() const { return m_textureCount; }
    
    bool enableNodeMetrics() const { return m_enableNodeMetrics; }
    void setEnableNodeMetrics(bool enable);
    
    bool enableTextureMetrics() const { return m_enableTextureMetrics; }
    void setEnableTextureMetrics(bool enable);

    // Add method to update metrics
    void updateMetricCounts(int nodes, int textures) {
        if (m_totalNodeCount != nodes || m_textureCount != textures) {
            m_totalNodeCount = nodes;
            m_textureCount = textures;
            qDebug() << "Metrics updated - Nodes:" << m_totalNodeCount 
                     << "Textures:" << m_textureCount;
        }
    }

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
    void startPositionXChanged();
    void moodImageSelected(const QString& url);  // Add this new signal
    void assetFocused(const QJsonObject& assetData);  // Modified to pass complete JSON object
    void enableNodeMetricsChanged();
    void enableTextureMetricsChanged();

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
    QMutex m_networkMutex;
    
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

    //QVector<ImageData> m_imageData;

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
        
        int index = -1;
        
        // Minimize mutex lock duration - just extract what we need
        {
            QMutexLocker locker(&m_networkMutex);
            index = m_pendingRequests.key(reply, -1);
            
            // Remove from pending requests map while under lock
            if (index != -1) {
                m_pendingRequests.remove(index);
            }
        }
        
        // Process the reply outside of mutex lock to avoid deadlocks
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

        reply->deleteLater();
    }

    void onNetworkError(QNetworkReply::NetworkError code) {
        QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
        if (!reply) return;
        
        int index = -1;
        
        // Minimize mutex lock duration
        {
            QMutexLocker locker(&m_networkMutex);
            index = m_pendingRequests.key(reply, -1);
            if (index != -1) {
                m_pendingRequests.remove(index);
            }
        }
        
        // Process outside the lock
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
