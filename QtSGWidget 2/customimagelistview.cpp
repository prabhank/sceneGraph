#include "customimagelistview.h"
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGTextureMaterial>
#include <QSGOpaqueTextureMaterial>
#include <QQuickWindow>
#include <QNetworkReply>
#include <QDebug>
#include <QSGSimpleTextureNode>
#include <QPainter>
#include <QFontMetrics>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QTimer>
#include <QSGFlatColorMaterial>
#include <cmath>
#include <QtMath>
#include "texturemanager.h"
#include <QGuiApplication>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslSocket>
#include <QtNetwork/QSslConfiguration>

//Q_LOGGING_CATEGORY(ihScheduleModel2, "custom", QtDebugMsg)

CustomImageListView::CustomImageListView(QQuickItem *parent)
    : QQuickItem(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    // Set up rendering flags
    setFlag(ItemHasContents, true);
    setFlag(QQuickItem::ItemIsFocusScope, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setFocus(true);
    setAcceptHoverEvents(true);
    setActiveFocusOnTab(true);

    // Set initial dimensions
    setHeight(300);
    setImplicitHeight(300);

    // Configure network manager
    setupNetworkManager();
    
    // Connect to window change signal with proper lambda capture
    connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow *w) {
        if (w) {
            // Capture the window pointer in the inner lambda
            connect(w, &QQuickWindow::beforeRendering, this, [this, w]() {
                if (!m_windowReady && w->isSceneGraphInitialized()) {
                    m_windowReady = true;
                    loadAllImages();
                }
            }, Qt::DirectConnection);
        }
    });
    
    // Initialize animation system
    setupScrollAnimation();
}

void CustomImageListView::componentComplete()
{
    QQuickItem::componentComplete();
    
    // Wait for scene graph initialization
    if (window()) {
        connect(window(), &QQuickWindow::beforeRendering, this, [this]() {
            if (!m_windowReady && window()->isExposed()) {
                m_windowReady = true;
                loadAllImages();
            }
        }, Qt::DirectConnection);
    }
    
    debugResourceSystem();
}

void CustomImageListView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChanged(newGeometry, oldGeometry);

    // Only reload when we have valid dimensions and window
    if (newGeometry.width() > 0 && newGeometry.height() > 0 && window()) {
        // Delay loading slightly to ensure window is ready
        QTimer::singleShot(100, this, [this]() {
            loadAllImages();
        });
    }
}

// Update loadAllImages method
void CustomImageListView::loadAllImages()
{
    if (!isReadyForTextures()) {
        QTimer::singleShot(100, this, [this]() {
            loadAllImages();
        });
        return;
    }

    // Calculate layout with reduced spacing
    qreal currentY = 0;
    int totalItems = 0;

    // Count total items and calculate height
    for (const QString &category : m_rowTitles) {
        int itemsInCategory = 0;
        for (const ImageData &imgData : m_imageData) {
            if (imgData.category == category) {
                itemsInCategory++;
                totalItems++;
            }
        }
        
        // Add category title height with reduced spacing
        currentY += m_titleHeight; // Using reduced title height
        
        // Add height for all items in category
        currentY += itemsInCategory * (m_itemHeight + m_spacing);
        
        // Add reduced spacing between categories
        currentY += m_rowSpacing;
    }

    // Load all images
    for (int i = 0; i < m_count; i++) {
        loadImage(i);
    }

    // Update content height
    setImplicitHeight(currentY);
    m_count = totalItems;
}

CustomImageListView::~CustomImageListView()
{
    m_isDestroying = true;
    
    // Cancel pending requests
    for (QNetworkReply *reply : m_pendingRequests) {
        if (reply) {
            reply->abort();
            reply->deleteLater();
        }
    }
    m_pendingRequests.clear();
    
    // Clear caches
    m_urlImageCache.clear();
    
    // Safely cleanup nodes
    safeReleaseTextures();
    
    // Cleanup animations
    stopCurrentAnimation();
    qDeleteAll(m_categoryAnimations);
    m_categoryAnimations.clear();
    delete m_scrollAnimation;
}

void CustomImageListView::safeReleaseTextures()
{
    QMutexLocker locker(&m_loadMutex);
    
    // Only delete nodes, not textures
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if (it.value().node) {
            delete it.value().node;
        }
    }
    m_nodes.clear();
}

void CustomImageListView::setCount(int count)
{
    if (m_count != count) {
        m_count = count;
        for(int i = 0; i < m_count; i++) {
            loadImage(i);
        }
        emit countChanged();
        update();
    }
}

void CustomImageListView::setItemWidth(qreal width)
{
    if (m_itemWidth != width) {
        m_itemWidth = width;
        emit itemWidthChanged();
        update();
    }
}

void CustomImageListView::setItemHeight(qreal height)
{
    if (m_itemHeight != height) {
        m_itemHeight = height;
        emit itemHeightChanged();
        update();
    }
}

void CustomImageListView::setSpacing(qreal spacing)
{
    if (m_spacing != spacing) {
        m_spacing = spacing;
        emit spacingChanged();
        update();
    }
}

void CustomImageListView::setRowSpacing(qreal spacing)
{
    if (m_rowSpacing != spacing) {
        m_rowSpacing = spacing;
        emit rowSpacingChanged();
        update();
    }
}

void CustomImageListView::setUseLocalImages(bool local)
{
    if (m_useLocalImages != local) {
        m_useLocalImages = local;
        emit useLocalImagesChanged();
        loadAllImages();
    }
}

void CustomImageListView::setImagePrefix(const QString &prefix)
{
    if (m_imagePrefix != prefix) {
        m_imagePrefix = prefix;
        emit imagePrefixChanged();
        if (m_useLocalImages) {
            loadAllImages();
        }
    }
}

void CustomImageListView::setImageUrls(const QVariantList &urls)
{
    if (m_imageUrls != urls) {
        m_imageUrls = urls;
        emit imageUrlsChanged();
        if (m_useLocalImages) {
            loadAllImages();
        }
    }
}

QString CustomImageListView::generateImageUrl(int index) const
{
    // Ensure minimum dimensions
    int imgWidth = qMax(int(m_itemWidth), 100);
    int imgHeight = qMax(int(height()), 100);
    
    QString url = QString("http://dummyimage.com/%1x%2/000/fff.jpg&text=img%3")
            .arg(imgWidth)
            .arg(imgHeight)
            .arg(index + 1);
            
    return url;
}

QImage CustomImageListView::loadLocalImage(int index) const
{
    QString imagePath;
    // Ensure index is within bounds of available images
    int actualIndex = (index % 5) + 1; // We have img1.jpg through img5.jpg
    
    // Update path to match new structure
    imagePath = QString(":/data/images/img%1.jpg").arg(actualIndex);
    
    qDebug() << "\nTrying to load image at index:" << index;
    qDebug() << "Using actual image path:" << imagePath;

    QFile file(imagePath);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray imageData = file.readAll();
        QImage image;
        if (image.loadFromData(imageData)) {
            qDebug() << "Successfully loaded image from:" << imagePath;
            qDebug() << "Image size:" << image.size();
            file.close();
            return image;
        }
        file.close();
    }

    // Create fallback image
    QImage fallback(m_itemWidth, m_itemHeight, QImage::Format_ARGB32);
    fallback.fill(Qt::red);
    QPainter painter(&fallback);
    painter.setPen(Qt::white);
    painter.drawText(fallback.rect(), Qt::AlignCenter, 
                    QString("Failed\nImage %1").arg(index + 1));
    return fallback;
}

QSGGeometryNode* CustomImageListView::createTexturedRect(const QRectF &rect, QSGTexture *texture, bool isFocused)
{
    // Calculate scale factor for focus effect
    const float scaleFactor = isFocused ? 1.1f : 1.0f;  // 10% larger when focused
    
    // Calculate scaled dimensions and center position
    QRectF scaledRect = rect;
    if (isFocused) {
        qreal widthDiff = rect.width() * (scaleFactor - 1.0f);
        qreal heightDiff = rect.height() * (scaleFactor - 1.0f);
        scaledRect = QRectF(
            rect.x() - widthDiff / 2,
            rect.y() - heightDiff / 2,
            rect.width() * scaleFactor,
            rect.height() * scaleFactor
        );
    }

    // Create geometry for a textured rectangle
    QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
    geometry->setDrawingMode(GL_TRIANGLE_STRIP);

    QSGGeometry::TexturedPoint2D *vertices = geometry->vertexDataAsTexturedPoint2D();
    
    // Set vertex positions using scaled rect
    vertices[0].set(scaledRect.left(), scaledRect.top(), 0.0f, 0.0f);
    vertices[1].set(scaledRect.right(), scaledRect.top(), 1.0f, 0.0f);
    vertices[2].set(scaledRect.left(), scaledRect.bottom(), 0.0f, 1.0f);
    vertices[3].set(scaledRect.right(), scaledRect.bottom(), 1.0f, 1.0f);

    QSGGeometryNode *node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);

    QSGOpaqueTextureMaterial *material = new QSGOpaqueTextureMaterial;
    material->setTexture(texture);
    
    node->setMaterial(material);
    node->setFlag(QSGNode::OwnsMaterial);

    return node;
}

// Add this helper method at the beginning of the class implementation
bool CustomImageListView::isReadyForTextures() const
{
    if (!m_windowReady || !window()) {
        return false;
    }
    
    // Check if window is exposed and visible
    if (!window()->isVisible() || !window()->isExposed()) {
        return false;
    }
    
    // Check window size
    if (window()->width() <= 0 || window()->height() <= 0) {
        return false;
    }
    
    return true;
}

void CustomImageListView::loadImage(int index)
{
    if (m_isDestroying || !ensureValidWindow() || index >= m_imageData.size()) {
        return;
    }

    QMutexLocker locker(&m_loadMutex);
    
    if (!isReadyForTextures()) {
        QTimer::singleShot(100, this, [this, index]() {
            loadImage(index);
        });
        return;
    }

    // Prevent duplicate texture creation
    if (m_nodes.contains(index) && m_nodes[index].texture) {
        return;
    }

    m_isLoading = true;

    // Load from URL
    const ImageData &imgData = m_imageData[index];
    QString imagePath = imgData.url;

    // First try to load as local resource
    QImage image = loadLocalImageFromPath(imagePath);
    if (!image.isNull()) {
        processLoadedImage(index, image);
        m_isLoading = false;
        return;
    }

    // If not a local resource, try as network URL
    QUrl url(imagePath);
    if (url.scheme().startsWith("http") || imagePath.startsWith("//")) {
        // Convert // URLs to http://
        if (imagePath.startsWith("//")) {
            url = QUrl("http:" + imagePath);
        }
        loadUrlImage(index, url);
    } else {
        createFallbackTexture(index);
    }

    m_isLoading = false;
}

bool CustomImageListView::ensureValidWindow() const
{
    return window() && window()->isExposed() && !m_isDestroying;
}

void CustomImageListView::createFallbackTexture(int index)
{
    // Create a basic RGB texture
    QImage fallback(m_itemWidth, m_itemHeight, QImage::Format_RGB32);
    fallback.fill(Qt::darkGray);
    
    QPainter painter(&fallback);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 14));
    painter.drawText(fallback.rect(), Qt::AlignCenter, 
                    QString("Image %1").arg(index + 1));
    painter.end();
    
    if (window()) {
        QSGTexture *texture = window()->createTextureFromImage(fallback);
        if (texture) {
            TexturedNode node;
            node.texture = texture;
            node.node = nullptr;
            m_nodes[index] = node;
            update();
            qDebug() << "Created fallback texture for index:" << index;
        }
    }
}

QImage CustomImageListView::loadLocalImageFromPath(const QString &path) const
{
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray imageData = file.readAll();
        QImage image;
        if (image.loadFromData(imageData)) {
            qDebug() << "Successfully loaded image from:" << path;
            qDebug() << "Image size:" << image.size();
            file.close();
            return image;
        }
        file.close();
    }

    qDebug() << "Failed to load image from:" << path;
    return QImage();
}

// Update loadUrlImage method to better handle HTTP requests
void CustomImageListView::loadUrlImage(int index, const QUrl &url)
{
    if (!url.isValid()) {
        qWarning() << "Invalid URL:" << url.toString();
        createFallbackTexture(index);
        return;
    }

    // Convert URL if needed (for URLs starting with //)
    QUrl finalUrl = url;
    if (url.toString().startsWith("//")) {
        finalUrl = QUrl("http:" + url.toString());
    }

    // For HTTP URLs
    if (finalUrl.scheme() == "http") {
        qDebug() << "Loading image" << index << "from URL:" << finalUrl.toString();
        
        // Create network request
        QNetworkRequest request(finalUrl);
        
        // Essential headers for Qt 5.6
        request.setRawHeader("User-Agent", "Mozilla/5.0 (compatible; Qt/5.6)");
        request.setRawHeader("Accept", "image/webp,image/apng,image/*,*/*;q=0.8");
        request.setRawHeader("Connection", "keep-alive");
        
        // Enable redirect following
        request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        
        // Cancel any existing request
        if (m_pendingRequests.contains(index)) {
            m_pendingRequests[index]->abort();
            m_pendingRequests[index]->deleteLater();
            m_pendingRequests.remove(index);
        }

        // Create and store network reply
        QNetworkReply *reply = m_networkManager->get(request);
        m_pendingRequests[index] = reply;
        
        // Connect signals with Qt 5.6 compatible syntax
        connect(reply, SIGNAL(finished()), this, SLOT(onNetworkReplyFinished()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), 
                this, SLOT(onNetworkError(QNetworkReply::NetworkError)));
        connect(reply, SIGNAL(sslErrors(QList<QSslError>)), 
                reply, SLOT(ignoreSslErrors()));
        
        // Add longer timeout for slower connections
        QTimer::singleShot(30000, reply, SLOT(abort()));
    } else {
        qWarning() << "Unsupported URL scheme:" << finalUrl.scheme();
        createFallbackTexture(index);
    }
}

void CustomImageListView::processLoadedImage(int index, const QImage &image)
{
    if (!image.isNull() && window()) {
        // Convert image to optimal format for textures
        QImage optimizedImage = image.convertToFormat(QImage::Format_RGBA8888);
        
        // Scale maintaining aspect ratio
        QImage scaledImage = optimizedImage.scaled(m_itemWidth, m_itemHeight,
                                                 Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation);
        
        // Create texture with proper flag (corrected syntax)
        QSGTexture *texture = window()->createTextureFromImage(
            scaledImage,
            QQuickWindow::TextureHasAlphaChannel
        );

        if (texture) {
            // Update the node map
            TexturedNode node;
            node.texture = texture;
            node.node = nullptr;
            m_nodes[index] = node;
            
            qDebug() << "Created texture for image" << index 
                     << "size:" << scaledImage.size();
            
            update(); // Request new frame
        }
    }
}

void CustomImageListView::setImageTitles(const QStringList &titles)
{
    if (m_imageTitles != titles) {
        m_imageTitles = titles;
        emit imageTitlesChanged();
        update();
    }
}

void CustomImageListView::setRowTitles(const QStringList &titles)
{
    if (m_rowTitles != titles) {
        m_rowTitles = titles;
        emit rowTitlesChanged();
        update();
    }
}

QSGGeometryNode* CustomImageListView::createOptimizedTextNode(const QString &text, const QRectF &rect)
{
    if (!window()) {
        return nullptr;
    }

    // Create new texture for text (don't use caching for now)
    int width = qMax(1, static_cast<int>(std::ceil(rect.width())));
    int height = qMax(1, static_cast<int>(std::ceil(rect.height())));
    
    QImage textImage(width, height, QImage::Format_ARGB32_Premultiplied);
    if (textImage.isNull()) {
        return nullptr;
    }
    
    textImage.fill(QColor(0, 0, 0, 0));
    
    QPainter painter;
    if (!painter.begin(&textImage)) {
        return nullptr;
    }

    static const QFont itemFont("Arial", 12);
    painter.setFont(itemFont);
    painter.setPen(Qt::white);
    painter.setRenderHints(QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter.drawText(textImage.rect(), Qt::AlignCenter, text);
    painter.end();
    
    QSGTexture *texture = window()->createTextureFromImage(
        textImage,
        QQuickWindow::TextureHasAlphaChannel
    );
    
    return texture ? createTexturedRect(rect, texture) : nullptr;
}

QSGGeometryNode* CustomImageListView::createRowTitleNode(const QString &text, const QRectF &rect)
{
    if (!window()) {
        return nullptr;
    }

    // Calculate title width based on text content
    static const QFont titleFont("Arial", 24, QFont::Bold);
    QFontMetrics metrics(titleFont);
    int textWidth = metrics.width(text) + 20;  // Add 20px padding
    int width = qMax(textWidth, static_cast<int>(std::ceil(rect.width())));
    int height = qMax(1, static_cast<int>(std::ceil(rect.height())));

    QImage textImage(width, height, QImage::Format_ARGB32_Premultiplied);
    if (textImage.isNull()) {
        qWarning() << "Failed to create text image with size:" << width << "x" << height;
        return nullptr;
    }

    textImage.fill(QColor(0, 0, 0, 0));

    QPainter painter;
    if (!painter.begin(&textImage)) {
        qWarning() << "Failed to start painting on image";
        return nullptr;
    }

    // Configure text rendering
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    painter.setRenderHints(QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    // Use classification ID as the row title
    QString displayText = text;
    if (displayText == "Default") {
        displayText = "Featured"; // Optional: provide friendly name for default category
    }
    
    // Draw text with left alignment and vertical centering
    int yPos = (height + metrics.ascent() - metrics.descent()) / 2;
    painter.drawText(10, yPos, displayText); // Add 10px left margin
    
    painter.end();

    // Create texture
    QSGTexture *texture = window()->createTextureFromImage(
        textImage,
        QQuickWindow::TextureHasAlphaChannel
    );

    if (!texture) {
        qWarning() << "Failed to create texture for row title";
        return nullptr;
    }

    // Create rect with adjusted width based on text content
    QRectF adjustedRect(rect.x(), rect.y(), textWidth, rect.height());
    return createTexturedRect(adjustedRect, texture);
}

// Fix the return type from void to QSGNode*
QSGNode* CustomImageListView::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (!window() || !window()->isExposed()) {
        delete oldNode;
        return nullptr;
    }
    
    QSGNode *parentNode = oldNode ? oldNode : new QSGNode;
    
    // Clear old nodes safely
    while (parentNode->childCount() > 0) {
        QSGNode *node = parentNode->firstChild();
        if (node) {
            parentNode->removeChildNode(node);
            delete node;
        }
    }
    
    qreal currentY = -m_contentY;
    int currentImageIndex = 0;
    
    // Create category containers
    for (int categoryIndex = 0; categoryIndex < m_rowTitles.size(); ++categoryIndex) {
        QString categoryName = m_rowTitles[categoryIndex];
        
        // Get dimensions for this category
        CategoryDimensions dims = getDimensionsForCategory(categoryName);
        
        // Calculate title width based on category name
        static const QFont titleFont("Arial", 24, QFont::Bold);
        QFontMetrics fm(titleFont);
        int titleWidth = fm.width(categoryName) + 20;  // Add 20px padding
        
        // Add 10-pixel offset for row title (increased from 5)
        QRectF titleRect(10, currentY, titleWidth, m_titleHeight);  // Changed from 5 to 10
        QSGGeometryNode *titleNode = createRowTitleNode(categoryName, titleRect);
        if (titleNode) {
            parentNode->appendChildNode(titleNode);
        }
        currentY += m_titleHeight + 10; // Changed from 5 to 10 for more spacing after title
        
        // Calculate items in this category
        QVector<ImageData> categoryItems;
        for (const ImageData &imgData : m_imageData) {
            if (imgData.category == categoryName) {
                categoryItems.append(imgData);
            }
        }

        // Add items using category-specific dimensions with 10-pixel offset (increased from 5)
        qreal xPos = 10 - getCategoryContentX(categoryName);  // Changed from 5 to 10
        for (const ImageData &imgData : categoryItems) {
            if (currentImageIndex < m_count) {
                QRectF rect(xPos, currentY, dims.posterWidth, dims.posterHeight);
                
                // Create item container
                QSGNode* itemContainer = new QSGNode;

                // Add image with focus effect
                if (m_nodes.contains(currentImageIndex) && m_nodes[currentImageIndex].texture) {
                    bool isFocused = (currentImageIndex == m_currentIndex);
                    QSGGeometryNode *imageNode = createTexturedRect(
                        rect, 
                        m_nodes[currentImageIndex].texture,
                        isFocused
                    );
                    if (imageNode) {
                        itemContainer->appendChildNode(imageNode);
                        m_nodes[currentImageIndex].node = imageNode;
                    }
                }

                // Add selection/focus effects if this is the current item
                if (currentImageIndex == m_currentIndex) {
                    addSelectionEffects(itemContainer, rect);
                }

                // Add title overlay
                addTitleOverlay(itemContainer, rect, imgData.title);

                // Add the container to parent
                parentNode->appendChildNode(itemContainer);
                currentImageIndex++;
                xPos += dims.posterWidth + dims.itemSpacing;
            }
        }
        
        currentY += dims.rowHeight + m_rowSpacing;
    }
    
    return parentNode;
}

// Helper method for selection effects
void CustomImageListView::addSelectionEffects(QSGNode* container, const QRectF& rect)
{
    // Calculate scaled dimensions for focus border to match zoomed asset
    const float scaleFactor = 1.1f;  // Match the zoom factor from createTexturedRect
    qreal widthDiff = rect.width() * (scaleFactor - 1.0f);
    qreal heightDiff = rect.height() * (scaleFactor - 1.0f);
    
    QRectF scaledRect(
        rect.x() - widthDiff / 2,
        rect.y() - heightDiff / 2,
        rect.width() * scaleFactor,
        rect.height() * scaleFactor
    );
    
    // Create white border effect
    QSGGeometryNode *borderNode = new QSGGeometryNode;
    
    // Create geometry for border (4 lines forming a rectangle)
    QSGGeometry *borderGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 8);
    borderGeometry->setDrawingMode(GL_LINES);
    borderGeometry->setLineWidth(2); // Border width
    
    QSGGeometry::Point2D *vertices = borderGeometry->vertexDataAsPoint2D();
    
    // Top line
    vertices[0].set(scaledRect.left(), scaledRect.top());
    vertices[1].set(scaledRect.right(), scaledRect.top());
    
    // Right line
    vertices[2].set(scaledRect.right(), scaledRect.top());
    vertices[3].set(scaledRect.right(), scaledRect.bottom());
    
    // Bottom line
    vertices[4].set(scaledRect.right(), scaledRect.bottom());
    vertices[5].set(scaledRect.left(), scaledRect.bottom());
    
    // Left line
    vertices[6].set(scaledRect.left(), scaledRect.bottom());
    vertices[7].set(scaledRect.left(), scaledRect.top());
    
    borderNode->setGeometry(borderGeometry);
    borderNode->setFlag(QSGNode::OwnsGeometry);
    
    // Create white material for border
    QSGFlatColorMaterial *borderMaterial = new QSGFlatColorMaterial;
    borderMaterial->setColor(QColor(Qt::white));
    
    borderNode->setMaterial(borderMaterial);
    borderNode->setFlag(QSGNode::OwnsMaterial);
    
    container->appendChildNode(borderNode);
}

// Helper method for title overlay
void CustomImageListView::addTitleOverlay(QSGNode* container, const QRectF& rect, const QString& title)
{
    QRectF textRect(rect.left(), rect.bottom() - 40, rect.width(), 40);
    
    // Add semi-transparent background
    QSGGeometryNode *textBgNode = new QSGGeometryNode;
    QSGGeometry *bgGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
    bgGeometry->setDrawingMode(GL_TRIANGLE_STRIP);
    
    QSGGeometry::Point2D *bgVertices = bgGeometry->vertexDataAsPoint2D();
    bgVertices[0].set(textRect.left(), textRect.top());
    bgVertices[1].set(textRect.right(), textRect.top());
    bgVertices[2].set(textRect.left(), textRect.bottom());
    bgVertices[3].set(textRect.right(), textRect.bottom());
    
    textBgNode->setGeometry(bgGeometry);
    textBgNode->setFlag(QSGNode::OwnsGeometry);
    
    QSGFlatColorMaterial *bgMaterial = new QSGFlatColorMaterial;
    bgMaterial->setColor(QColor(0, 0, 0, 128));
    textBgNode->setMaterial(bgMaterial);
    textBgNode->setFlag(QSGNode::OwnsMaterial);
    
    container->appendChildNode(textBgNode);

    // Add text
    QSGGeometryNode *textNode = createOptimizedTextNode(title, textRect);
    if (textNode) {
        container->appendChildNode(textNode);
    }
}

// Add this new function
void CustomImageListView::debugResourceSystem() const 
{
    qDebug() << "\n=== Resource System Debug ===";
    qDebug() << "Application dir:" << QCoreApplication::applicationDirPath();
    qDebug() << "Current working directory:" << QDir::currentPath();
    
    // Check resource root
    QDir resourceRoot(":/");
    qDebug() << "\nResource root contents:";
    for(const QString &entry : resourceRoot.entryList()) {
        qDebug() << " -" << entry;
    }
    
    // Check images directory
    QDir imagesDir(":/images");
    qDebug() << "\nImages directory contents:";
    for(const QString &entry : imagesDir.entryList()) {
        qDebug() << " -" << entry;
    }
    
    // Try to open each image path variation
    QString testPath = "/images/img1.jpg";
    QStringList pathsToTest;
    pathsToTest << testPath
                << ":" + testPath
                << "qrc:" + testPath
                << QCoreApplication::applicationDirPath() + testPath;
    
    qDebug() << "\nTesting image paths:";
    for(const QString &path : pathsToTest) {
        qDebug() << "\nTesting path:" << path;
        QFile file(path);
        if (file.exists()) {
            qDebug() << " - File exists";
            if (file.open(QIODevice::ReadOnly)) {
                qDebug() << " - Can open file";
                qDebug() << " - File size:" << file.size();
                file.close();
            } else {
                qDebug() << " - Cannot open file:" << file.errorString();
            }
        } else {
            qDebug() << " - File does not exist";
        }
    }
}

void CustomImageListView::itemChange(ItemChange change, const ItemChangeData &data)
{
    QQuickItem::itemChange(change, data);
    if (change == ItemSceneChange) {
        m_windowReady = (data.window != nullptr);
        if (m_windowReady) {
            initializeGL();  // Initialize OpenGL when window is ready
            tryLoadImages();
        }
    }
}

void CustomImageListView::tryLoadImages()
{
    if (!m_windowReady || !window()) {
        qDebug() << "Window not ready, deferring image loading";
        return;
    }
    
    if (width() <= 0 || height() <= 0) {
        qDebug() << "Invalid dimensions, deferring image loading";
        return;
    }

    loadAllImages();
}

void CustomImageListView::setCurrentIndex(int index)
{
    if (m_currentIndex != index && index >= 0 && index < m_count) {
        m_currentIndex = index;
        updateCurrentCategory();
        emit currentIndexChanged();
        update();
    }
}

void CustomImageListView::keyPressEvent(QKeyEvent *event)
{
    qDebug() << "Key pressed:" << event->key() << "Has focus:" << hasActiveFocus();
    
    switch (event->key()) {
        case Qt::Key_Left:
            navigateLeft();
            event->accept();
            break;
        case Qt::Key_Right:
            navigateRight();
            event->accept();
            break;
        case Qt::Key_Up:
            navigateUp();
            event->accept();
            break;
        case Qt::Key_Down:
            navigateDown();
            event->accept();
            break;
        default:
            QQuickItem::keyPressEvent(event);
    }
}

void CustomImageListView::navigateLeft()
{
    QString currentCategory = m_imageData[m_currentIndex].category;
    int prevIndex = m_currentIndex - 1;
    
    while (prevIndex >= 0) {
        if (m_imageData[prevIndex].category != currentCategory) {
            break;
        }
        setCurrentIndex(prevIndex);
        ensureIndexVisible(prevIndex);
        ensureFocus();
        updateCurrentCategory();
        update();
        return;
    }
}

void CustomImageListView::navigateRight()
{
    QString currentCategory = m_imageData[m_currentIndex].category;
    int nextIndex = m_currentIndex + 1;
    
    while (nextIndex < m_imageData.size()) {
        if (m_imageData[nextIndex].category != currentCategory) {
            break;
        }
        
        // Get category dimensions and calculate if scrolling is needed
        CategoryDimensions dims = getDimensionsForCategory(currentCategory);
        int itemsInCategory = 0;
        int lastIndexInCategory = nextIndex;
        
        // Count items in this category and find last index
        for (int i = 0; i < m_imageData.size(); i++) {
            if (m_imageData[i].category == currentCategory) {
                itemsInCategory++;
                lastIndexInCategory = i;
            }
        }
        
        setCurrentIndex(nextIndex);
        ensureIndexVisible(nextIndex);
        ensureFocus();
        updateCurrentCategory();
        
        // Calculate scroll target position with animation
        qreal totalWidth = itemsInCategory * dims.posterWidth + 
                          (itemsInCategory - 1) * dims.itemSpacing;
                          
        if (totalWidth > width()) {
            qreal targetX;
            if (nextIndex == lastIndexInCategory) {
                targetX = categoryContentWidth(currentCategory) - width();
            } else {
                int itemsBeforeInCategory = 0;
                for (int i = 0; i < nextIndex; i++) {
                    if (m_imageData[i].category == currentCategory) {
                        itemsBeforeInCategory++;
                    }
                }
                targetX = itemsBeforeInCategory * (dims.posterWidth + dims.itemSpacing);
                targetX = qMax(0.0, targetX - (width() - dims.posterWidth) / 2);
            }
            
            // Animate to target position
            animateScroll(currentCategory, targetX);
        }
        
        update();
        return;
    }
}

// Update navigateUp and navigateDown methods
void CustomImageListView::navigateUp()
{
    if (m_currentIndex < 0 || m_rowTitles.isEmpty()) {
        return;
    }

    QString currentCategory = m_imageData[m_currentIndex].category;
    int categoryIndex = m_rowTitles.indexOf(currentCategory);
    
    if (categoryIndex > 0) {
        QString prevCategory = m_rowTitles[categoryIndex - 1];
        CategoryDimensions prevDims = getDimensionsForCategory(prevCategory);
        
        // Get current horizontal scroll position for the target category
        qreal categoryScrollX = getCategoryContentX(prevCategory);
        
        // Find first visible item in the previous category
        int firstVisibleIndex = -1;
        qreal bestDistance = std::numeric_limits<qreal>::max();
        
        for (int i = 0; i < m_imageData.size(); i++) {
            if (m_imageData[i].category == prevCategory) {
                // Calculate item's x position
                int itemsBeforeThis = 0;
                for (int j = 0; j < i; j++) {
                    if (m_imageData[j].category == prevCategory) {
                        itemsBeforeThis++;
                    }
                }
                
                qreal itemX = itemsBeforeThis * (prevDims.posterWidth + prevDims.itemSpacing);
                
                // Check if item is visible (considering scroll position)
                if (itemX >= categoryScrollX && 
                    itemX < (categoryScrollX + width() - prevDims.posterWidth)) {
                    qreal distanceFromLeft = itemX - categoryScrollX;
                    if (distanceFromLeft < bestDistance) {
                        bestDistance = distanceFromLeft;
                        firstVisibleIndex = i;
                    }
                }
            }
        }
        
        // If we found a visible item, select it
        if (firstVisibleIndex != -1) {
            setCurrentIndex(firstVisibleIndex);
            ensureIndexVisible(firstVisibleIndex);
            ensureFocus();
            update();
        } else {
            // Fallback to first item in category if none are visible
            for (int i = 0; i < m_imageData.size(); i++) {
                if (m_imageData[i].category == prevCategory) {
                    setCurrentIndex(i);
                    ensureIndexVisible(i);
                    ensureFocus();
                    update();
                    break;
                }
            }
        }
    }
}

void CustomImageListView::navigateDown()
{
    if (m_currentIndex < 0 || m_rowTitles.isEmpty()) {
        return;
    }

    QString currentCategory = m_imageData[m_currentIndex].category;
    int categoryIndex = m_rowTitles.indexOf(currentCategory);
    
    if (categoryIndex < m_rowTitles.size() - 1) {
        QString nextCategory = m_rowTitles[categoryIndex + 1];
        CategoryDimensions nextDims = getDimensionsForCategory(nextCategory);
        
        // Get current horizontal scroll position for the target category
        qreal categoryScrollX = getCategoryContentX(nextCategory);
        
        // Find first visible item in the next category
        int firstVisibleIndex = -1;
        qreal bestDistance = std::numeric_limits<qreal>::max();
        
        for (int i = 0; i < m_imageData.size(); i++) {
            if (m_imageData[i].category == nextCategory) {
                // Calculate item's x position
                int itemsBeforeThis = 0;
                for (int j = 0; j < i; j++) {
                    if (m_imageData[j].category == nextCategory) {
                        itemsBeforeThis++;
                    }
                }
                
                qreal itemX = itemsBeforeThis * (nextDims.posterWidth + nextDims.itemSpacing);
                
                // Check if item is visible (considering scroll position)
                if (itemX >= categoryScrollX && 
                    itemX < (categoryScrollX + width() - nextDims.posterWidth)) {
                    qreal distanceFromLeft = itemX - categoryScrollX;
                    if (distanceFromLeft < bestDistance) {
                        bestDistance = distanceFromLeft;
                        firstVisibleIndex = i;
                    }
                }
            }
        }
        
        // If we found a visible item, select it
        if (firstVisibleIndex != -1) {
            setCurrentIndex(firstVisibleIndex);
            ensureIndexVisible(firstVisibleIndex);
            ensureFocus();
            update();
        } else {
            // Fallback to first item in category if none are visible
            for (int i = 0; i < m_imageData.size(); i++) {
                if (m_imageData[i].category == nextCategory) {
                    setCurrentIndex(i);
                    ensureIndexVisible(i);
                    ensureFocus();
                    update();
                    break;
                }
            }
        }
    }
}

// Add this helper method to calculate item's vertical position
qreal CustomImageListView::calculateItemVerticalPosition(int index)
{
    if (index < 0 || index >= m_imageData.size()) {
        return 0;
    }

    qreal position = 0;
    QString targetCategory = m_imageData[index].category;
    
    for (const QString &category : m_rowTitles) {
        CategoryDimensions dims = getDimensionsForCategory(category);
        
        // Add spacing between categories
        if (position > 0) {
            position += m_rowSpacing;
        }

        // Add title height
        position += m_titleHeight;

        if (category == targetCategory) {
            break;
        }

        // Add row height
        position += dims.rowHeight;
    }

    return position;
}

void CustomImageListView::ensureIndexVisible(int index)
{
    if (index < 0 || index >= m_imageData.size()) {
        return;
    }

    QString targetCategory = m_imageData[index].category;
    CategoryDimensions dims = getDimensionsForCategory(targetCategory);
    
    // Calculate vertical position
    qreal targetY = calculateItemVerticalPosition(index);
    
    // Center vertically with proper offset
    qreal centerOffset = (height() - dims.posterHeight) / 2;
    qreal newContentY = targetY - centerOffset;
    
    // Bound the scroll position
    newContentY = qBound(0.0, newContentY, qMax(0.0, contentHeight() - height()));
    setContentY(newContentY);
    
    // Handle horizontal scrolling
    int itemsBeforeInCategory = 0;
    for (int i = 0; i < index; i++) {
        if (m_imageData[i].category == targetCategory) {
            itemsBeforeInCategory++;
        }
    }
    
    // Calculate x position considering item dimensions
    qreal xOffset = itemsBeforeInCategory * (dims.posterWidth + dims.itemSpacing);
    
    // Center the item horizontally with proper offset
    qreal targetX = xOffset - (width() - dims.posterWidth) / 2;
    
    // Add extra space to ensure focus border is visible
    qreal extraSpace = 10; // Pixels for focus border visibility
    
    // Calculate maximum scroll considering item width and spacing
    qreal maxScroll = categoryContentWidth(targetCategory) - width();
    
    // Bound the scroll position with extra space consideration
    targetX = qBound(0.0, targetX, maxScroll + extraSpace);
    
    setCategoryContentX(targetCategory, targetX);
}

// Add helper method to calculate category width
qreal CustomImageListView::categoryContentWidth(const QString& category) const
{
    // Get the correct dimensions for this category
    CategoryDimensions dims = getDimensionsForCategory(category);
    
    // Count items in this category
    int itemCount = 0;
    for (const ImageData &imgData : m_imageData) {
        if (imgData.category == category) {
            itemCount++;
        }
    }
    
    // Calculate total width using category-specific dimensions
    qreal totalWidth = itemCount * dims.posterWidth + 
                       (itemCount - 1) * dims.itemSpacing;
                       
    // Only add padding if the content is wider than the view
    if (totalWidth > width()) {
        totalWidth += dims.posterWidth * 0.5; // Add half poster width as padding
    }
    
    return totalWidth;
}

bool CustomImageListView::event(QEvent *e)
{
    if (e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut) {
        update();
    }
    return QQuickItem::event(e);
}

void CustomImageListView::mousePressEvent(QMouseEvent *event)
{
    ensureFocus();
    QQuickItem::mousePressEvent(event);
}

void CustomImageListView::ensureFocus()
{
    if (!hasActiveFocus()) {
        setFocus(true);
        forceActiveFocus();
    }
}

void CustomImageListView::setLocalImageUrls(const QVariantList &urls)
{
    if (m_localImageUrls != urls) {
        m_localImageUrls = urls;
        emit localImageUrlsChanged();
        loadAllImages();
    }
}

void CustomImageListView::setRemoteImageUrls(const QVariantList &urls)
{
    if (m_remoteImageUrls != urls) {
        m_remoteImageUrls = urls;
        emit remoteImageUrlsChanged();
        loadAllImages();
    }
}

void CustomImageListView::setRowCount(int count)
{
    if (m_rowCount != count) {
        m_rowCount = count;
        emit rowCountChanged();
        update();
    }
}

void CustomImageListView::setContentX(qreal x)
{
    if (m_contentX != x) {
        m_contentX = x;
        emit contentXChanged();
        update();
    }
}

void CustomImageListView::setContentY(qreal y)
{
    if (m_contentY != y) {
        m_contentY = y;
        emit contentYChanged();
        update();
    }
}

qreal CustomImageListView::contentWidth() const
{
    return m_itemsPerRow * m_itemWidth + (m_itemsPerRow - 1) * m_spacing;
}

// Update contentHeight calculation for tighter spacing
qreal CustomImageListView::contentHeight() const
{
    qreal totalHeight = 0;
    
    // Add minimal initial padding
    totalHeight += m_rowSpacing / 2;  // Reduced padding
    
    for (int categoryIndex = 0; categoryIndex < m_rowTitles.size(); ++categoryIndex) {
        // Add minimal spacing between categories
        if (categoryIndex > 0) {
            totalHeight += m_rowSpacing;  // Reduced from double spacing
        }

        // Add category title height
        totalHeight += m_titleHeight;

        QString categoryName = m_rowTitles[categoryIndex];
        int categoryImageCount = 0;
        for (const ImageData &imgData : m_imageData) {
            if (imgData.category == categoryName) {
                categoryImageCount++;
            }
        }

        // Calculate rows needed for this category
        int rowsInCategory = (categoryImageCount + m_itemsPerRow - 1) / m_itemsPerRow;
        
        // Add height for all rows in this category
        totalHeight += rowsInCategory * m_itemHeight;
        
        // Add minimal spacing between rows within category
        if (rowsInCategory > 1) {
            totalHeight += (rowsInCategory - 1) * (m_rowSpacing / 2);  // Reduced internal row spacing
        }
    }

    // Add minimal padding at bottom
    totalHeight += m_rowSpacing / 2;  // Reduced padding
    
    return totalHeight;
}

// Update wheelEvent to handle per-category scrolling
void CustomImageListView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ShiftModifier) {
        // Horizontal scrolling within current category
        QString currentCategory = m_imageData[m_currentIndex].category;
        qreal horizontalDelta = event->angleDelta().x() / 120.0 * m_itemWidth;
        qreal newX = m_contentX - horizontalDelta;
        newX = qBound(0.0, newX, qMax(0.0, categoryContentWidth(currentCategory) - width()));
        setCategoryContentX(currentCategory, newX);
    } else {
        // Vertical scrolling between categories
        qreal verticalDelta = event->angleDelta().y() / 120.0 * m_itemHeight;
        qreal newY = m_contentY - verticalDelta;
        newY = qBound(0.0, newY, qMax(0.0, contentHeight() - height()));
        setContentY(newY);
    }
    
    event->accept();
}

void CustomImageListView::cleanupNode(TexturedNode& node)
{
    if (node.node) {
        delete node.node;
        node.node = nullptr;
    }
    // Don't delete texture, just clear the pointer
    node.texture = nullptr;
}

void CustomImageListView::limitTextureCacheSize(int maxTextures)
{
    if (m_nodes.size() > maxTextures) {
        // Remove oldest textures
        auto it = m_nodes.begin();
        while (m_nodes.size() > maxTextures && it != m_nodes.end()) {
            cleanupNode(it.value());
            it = m_nodes.erase(it);
        }
    }
}

// Update cleanupTextures
void CustomImageListView::cleanupTextures()
{
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        cleanupNode(it.value());
    }
    m_nodes.clear();
}

void CustomImageListView::setJsonSource(const QUrl &source)
{
    if (m_jsonSource != source) {
        m_jsonSource = source;
        loadFromJson(source);
        emit jsonSourceChanged();
    }
}

void CustomImageListView::loadFromJson(const QUrl &source)
{
    qDebug() << "Loading JSON from source:" << source.toString();
    
    // First load UI settings
    loadUISettings();
    
    // Then load menu data
    QString menuPath;
    if (source.scheme() == "qrc") {
        menuPath = ":" + source.path();
        qDebug() << "Converted QRC path to:" << menuPath;
    } else {
        menuPath = source.toLocalFile();
        qDebug() << "Using local file path:" << menuPath;
    }
    
    QFile menuFile(menuPath);
    qDebug() << "Attempting to open file:" << menuPath;
    qDebug() << "File exists:" << menuFile.exists();
    
    if (!menuFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to load menu data from:" << menuPath 
                   << "Error:" << menuFile.errorString();
        
        // Try alternative path
        QString altPath = ":/data/embeddedHubMenu.json";
        QFile altFile(altPath);
        qDebug() << "Trying alternative path:" << altPath 
                 << "Exists:" << altFile.exists();
        
        if (altFile.open(QIODevice::ReadOnly)) {
            QByteArray data = altFile.readAll();
            qDebug() << "Successfully read alternative file, size:" << data.size();
            processJsonData(data);
            altFile.close();
        } else {
            qWarning() << "Failed to load menu data from alternative path:" << altPath 
                      << "Error:" << altFile.errorString();
        }
    } else {
        QByteArray data = menuFile.readAll();
        qDebug() << "Successfully read file, size:" << data.size();
        menuFile.close();
        processJsonData(data);
    }
}

// Update loadUISettings method
void CustomImageListView::loadUISettings()
{
    QString settingsPath = ":/data/uiSettings.json";
    QFile settingsFile(settingsPath);
    
    if (settingsFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(settingsFile.readAll());
        settingsFile.close();
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonObject uiConfig = root["uiConfigurations"].toObject();
            QJsonObject stbConfig = uiConfig["STB"].toObject();
            QJsonObject kv3Config = stbConfig["kv3"].toObject();
            
            // Get swimlane configuration
            QJsonObject swimlaneConfig = kv3Config["swimlaneSizeConfiguration"].toObject();
            QJsonObject posterWithMetaData = swimlaneConfig["posterWithMetaData"].toObject();
            
            // Load category-specific dimensions with reduced spacing
            if (posterWithMetaData.contains("portraitType1")) {
                QJsonObject portraitConfig = posterWithMetaData["portraitType1"].toObject();
                
                CategoryDimensions dims;
                dims.rowHeight = portraitConfig["height"].toInt(240);      // Reduced from 284
                dims.posterHeight = portraitConfig["posterHeight"].toInt(200); // Reduced from 228
                dims.posterWidth = portraitConfig["posterWidth"].toInt(152);
                dims.itemSpacing = 10;  // Reduced from 15
                
                m_categoryDimensions["Bein Series"] = dims;
            }
            
            // Set reduced row spacing
            setRowSpacing(15);  // Reduced from 20
            
            // Set reduced row title height
            m_titleHeight = 30; // Reduced from 30
        }
    }
}

void CustomImageListView::processJsonData(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON data - not an object";
        return;
    }

    QJsonObject root = doc.object();
    
    // Get menuItems object first
    QJsonObject menuItems = root["menuItems"].toObject();
    if (menuItems.isEmpty()) {
        qWarning() << "menuItems object is empty or invalid";
        return;
    }

    // Clear existing data
    m_imageData.clear();
    m_rowTitles.clear();

    // Process the outer items array (rows)
    QJsonArray rows = menuItems["items"].toArray();
    for (const QJsonValue &rowVal : rows) {
        QJsonObject row = rowVal.toObject();
        
        // Get row information
        QString classificationId = row["classificationId"].toString();
        QString rowTitle = row["title"].toString();
        
        // Add row title
        m_rowTitles.append(rowTitle);
        
        // Process items in this row
        QJsonArray items = row["items"].toArray();
        for (const QJsonValue &itemVal : items) {
            if (!itemVal.isObject()) continue;
            
            QJsonObject item = itemVal.toObject();
            
            // Skip "viewAll" type items
            if (item["assetType"].toString() == "viewAll") {
                continue;
            }
            
            ImageData imgData;
            imgData.category = rowTitle;  // Use row title as category
            imgData.title = item["title"].toString();
            
            // Get image URL - prioritize moodImageUri then thumbnailUri
            imgData.url = item["moodImageUri"].toString();
            if (imgData.url.isEmpty()) {
                imgData.url = item["thumbnailUri"].toString();
            }
            
            // Add additional metadata
            imgData.id = item["assetType"].toString();
            imgData.description = item["shortSynopsis"].toString();
            
            // Clean up URL if needed
            if (imgData.url.startsWith("//")) {
                imgData.url = "https:" + imgData.url;
            }
            
            // Use default image if no URL
            if (imgData.url.isEmpty()) {
                int index = m_imageData.size() % 5 + 1;
                imgData.url = QString(":/data/images/img%1.jpg").arg(index);
            }
            
            qDebug() << "Adding item to row" << rowTitle 
                     << "\n  - Title:" << imgData.title
                     << "\n  - URL:" << imgData.url;
            
            m_imageData.append(imgData);
        }
    }

    // Update view
    m_count = m_imageData.size();
    if (m_count > 0) {
        safeReleaseTextures();
        loadAllImages();
        emit countChanged();
        emit rowTitlesChanged();
        update();
    } else {
        qWarning() << "No menu items were loaded!";
        addDefaultItems();
    }
}

void CustomImageListView::addDefaultItems()
{
    qDebug() << "Adding default test items";
    
    m_rowTitles.clear();
    m_rowTitles.append("Test Items");
    
    for (int i = 0; i < 5; i++) {
        ImageData imgData;
        imgData.category = "Test Items";
        imgData.title = QString("Test Item %1").arg(i + 1);
        imgData.url = QString(":/data/images/img%1.jpg").arg(i % 5 + 1);
        imgData.id = QString::number(i);
        m_imageData.append(imgData);
    }
    
    m_count = m_imageData.size();
    safeReleaseTextures();
    loadAllImages();
    emit countChanged();
    emit rowTitlesChanged();
    update();
}

void CustomImageListView::initializeGL()
{
    if (!window()) return;

    // Set environment variables for debugging
    qputenv("QSG_INFO", "1");
    qputenv("QT_LOGGING_RULES", "qt.scenegraph.general=true");

    // Set OpenGL attributes
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // Force software rendering for embedded systems
    qputenv("QT_QUICK_BACKEND", "software");
    
    // Connect to window's sceneGraphInitialized signal
    connect(window(), &QQuickWindow::beforeRendering, this, [this]() {
        // Verify OpenGL context
        QOpenGLContext *context = window()->openglContext();
        if (context) {
            qDebug() << "OpenGL Version:" << context->format().majorVersion() 
                     << "." << context->format().minorVersion();
            qDebug() << "Using OpenGL:" << context->isValid();
            
            // Set up surface format
            QSurfaceFormat format = context->format();
            format.setRenderableType(QSurfaceFormat::OpenGLES);
            format.setVersion(2, 0);
            format.setSwapInterval(1);
            format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
            window()->setFormat(format);
        }
    }, Qt::DirectConnection);
}

// Add helper methods for per-category scrolling
void CustomImageListView::setCategoryContentX(const QString& category, qreal x)
{
    m_categoryContentX[category] = x;
    update();
}

qreal CustomImageListView::getCategoryContentX(const QString& category) const
{
    return m_categoryContentX.value(category, 0.0);
}

void CustomImageListView::updateCurrentCategory()
{
    if (m_currentIndex >= 0 && m_currentIndex < m_imageData.size()) {
        m_currentCategory = m_imageData[m_currentIndex].category;
    }
}

// Add safety check helper method
void CustomImageListView::ensureValidIndex(int &index) 
{
    if (index < 0) {
        index = 0;
    }
    if (index >= m_imageData.size()) {
        index = m_imageData.size() - 1;
    }
}

// Add in constructor after m_networkManager initialization
void CustomImageListView::setupNetworkManager()
{
    if (!m_networkManager) return;
    
    // Configure network manager for Qt 5.6
    m_networkManager->setConfiguration(QNetworkConfiguration());
    m_networkManager->setNetworkAccessible(QNetworkAccessManager::Accessible);

    // Enable SSL
    #ifndef QT_NO_SSL
        QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
        sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
        sslConfig.setProtocol(QSsl::TlsV1_0);
        QSslConfiguration::setDefaultConfiguration(sslConfig);
    #endif
}

void CustomImageListView::setupScrollAnimation()
{
    // Create a reusable animation object
    m_scrollAnimation = new QPropertyAnimation(this);
    m_scrollAnimation->setDuration(300);  // 300ms duration
    m_scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void CustomImageListView::animateScroll(const QString& category, qreal targetX)
{
    // Stop any existing animation
    stopCurrentAnimation();
    
    // Create a new animation for this category if it doesn't exist
    if (!m_categoryAnimations.contains(category)) {
        QPropertyAnimation* anim = new QPropertyAnimation(this);
        anim->setDuration(300);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        m_categoryAnimations[category] = anim;
        
        // Connect animation to update the scroll position
        connect(anim, &QPropertyAnimation::valueChanged, this, [this, category](const QVariant& value) {
            setCategoryContentX(category, value.toReal());
        });
    }
    
    QPropertyAnimation* anim = m_categoryAnimations[category];
    anim->stop();
    anim->setStartValue(getCategoryContentX(category));
    anim->setEndValue(targetX);
    anim->start();
}

void CustomImageListView::stopCurrentAnimation()
{
    // Stop all category animations
    for (QPropertyAnimation* anim : m_categoryAnimations.values()) {
        if (anim->state() == QPropertyAnimation::Running) {
            anim->stop();
        }
    }
}

