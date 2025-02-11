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

// Update the constructor to initialize proper dimensions
CustomImageListView::CustomImageListView(QQuickItem *parent)
    : QQuickItem(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    setFlag(ItemHasContents, true);
    setFlag(QQuickItem::ItemIsFocusScope, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setFocus(true);
    setAcceptHoverEvents(true);
    setActiveFocusOnTab(true);

    // Set initial height - use setHeight instead of setMinimumHeight for QQuickItem
    setHeight(300);
    setImplicitHeight(300);  // Also set implicit height for proper sizing in QML
    // Don't load images in constructor - wait for componentComplete
}

void CustomImageListView::componentComplete()
{
    QQuickItem::componentComplete();
    
    // Wait for scene graph initialization
    if (window()) {
        connect(window(), &QQuickWindow::sceneGraphInitialized, this, [this]() {
            m_windowReady = true;
            QTimer::singleShot(100, this, &CustomImageListView::loadAllImages);
        });

        connect(window(), &QQuickWindow::beforeRendering, this, [this]() {
            if (!m_windowReady && window()->isSceneGraphInitialized()) {
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

    // Avoid loading images when width or height is zero
    if (newGeometry.width() < 1 || newGeometry.height() < 1) {
        qDebug() << "Invalid geometry, skipping loadAllImages:";
        return;
    }

    // Skip re-load if another load is in progress
    if (m_isLoading) {
        qDebug() << "Image loading in progress, skipping reload";
        return;
    }
    
    if (newGeometry.size() != oldGeometry.size()) {
        qDebug() << "Geometry changed, reloading images with new size:" << newGeometry.size();
        loadAllImages();
    }
}

void CustomImageListView::loadAllImages()
{
    if (!isReadyForTextures()) {
        QTimer::singleShot(100, this, [this]() {
            loadAllImages();
        });
        return;
    }

    // Remove the call to cleanupTextures() here to avoid double frees
    // cleanupTextures();  // <-- Remove this line

    // Clear existing nodes and textures
    // Remove node deletion here as well (avoid double deletion):
    /*
    for (auto &node : m_nodes) {
        delete node.node;
        delete node.texture;
    }
    m_nodes.clear();
    */

    // Calculate total rows needed
    m_itemsPerRow = qMax(1, int((width() + m_spacing) / (m_itemWidth + m_spacing)));
    int totalRows = (m_count + m_itemsPerRow - 1) / m_itemsPerRow;
    
    // Pre-calculate row heights including titles and spacing
    qreal currentY = 0;
    for (int row = 0; row < totalRows; ++row) {
        currentY += (row == 0 ? 0 : m_rowSpacing); // Add spacing between rows
        if (row < m_rowTitles.size()) {
            currentY += 40; // Row title height
        }
        
        // Load images for this row
        for (int col = 0; col < m_itemsPerRow; ++col) {
            int index = row * m_itemsPerRow + col;
            if (index < m_count) {
                loadImage(index);
            }
        }
        
        currentY += m_itemHeight; // Add item height
    }

    // Update content height
    setImplicitHeight(currentY);
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
    
    imagePath = QString(":/images/img%1.jpg").arg(actualIndex);
    
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

QSGGeometryNode* CustomImageListView::createTexturedRect(const QRectF &rect, QSGTexture *texture)
{
    // Create geometry for a textured rectangle
    QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
    geometry->setDrawingMode(GL_TRIANGLE_STRIP);

    QSGGeometry::TexturedPoint2D *vertices = geometry->vertexDataAsTexturedPoint2D();
    
    // Set vertex positions
    vertices[0].set(rect.left(), rect.top(), 0.0f, 0.0f);
    vertices[1].set(rect.right(), rect.top(), 1.0f, 0.0f);
    vertices[2].set(rect.left(), rect.bottom(), 0.0f, 1.0f);
    vertices[3].set(rect.right(), rect.bottom(), 1.0f, 1.0f);

    // Create node with geometry
    QSGGeometryNode *node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);

    // Setup material
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
    
    // Check if window is exposed
    if (!window()->isVisible()) {
        return false;
    }
    
    // Check window size
    if (window()->width() <= 0 || window()->height() <= 0) {
        return false;
    }
    
    // Check if scene graph is initialized
    if (!window()->isSceneGraphInitialized()) {
        return false;
    }
    
    return true;
}

void CustomImageListView::loadImage(int index)
{
    if (m_isDestroying || !ensureValidWindow()) {
        return;
    }

    QMutexLocker locker(&m_loadMutex);
    
    if (!isReadyForTextures() || m_isLoading) {
        QTimer::singleShot(100, this, [this, index]() {
            loadImage(index);
        });
        return;
    }

    m_isLoading = true;

    // Clear any existing node (not texture)
    if (m_nodes.contains(index)) {
        cleanupNode(m_nodes[index]);
    }

    int imageIndex = (index % 5) + 1;
    QString imagePath = QString(":/images/img%1.jpg").arg(imageIndex);
    
    QSGTexture* texture = TextureBuffer::instance().acquire(window(), imagePath);
    if (texture) {
        TexturedNode node;
        node.texture = texture;  // Just store the pointer
        node.node = nullptr;
        node.index = index;
        m_nodes[index] = node;
        update();
        qDebug() << "Successfully loaded image" << index;
    } else {
        qWarning() << "Failed to load image" << index << ", creating fallback";
        createFallbackTexture(index);
    }

    m_isLoading = false;
}

bool CustomImageListView::ensureValidWindow() const
{
    return window() && window()->isSceneGraphInitialized() && !m_isDestroying;
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

void CustomImageListView::loadUrlImage(int index, const QUrl &url)
{
    // Check cache first
    if (m_urlImageCache.contains(url)) {
        QImage image = m_urlImageCache[url];
        processLoadedImage(index, image);
        return;
    }

    // Cancel any existing request for this index
    if (m_pendingRequests.contains(index)) {
        m_pendingRequests[index]->abort();
        m_pendingRequests[index]->deleteLater();
        m_pendingRequests.remove(index);
    }

    // Start new request
    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);
    m_pendingRequests[index] = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply, index]() {
        handleNetworkReply(reply, index);
    });
}

void CustomImageListView::handleNetworkReply(QNetworkReply *reply, int index)
{
    reply->deleteLater();
    m_pendingRequests.remove(index);

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Network error:" << reply->errorString();
        // Load fallback image
        QImage fallback(m_itemWidth, height(), QImage::Format_ARGB32);
        fallback.fill(Qt::red);
        QPainter painter(&fallback);
        painter.setPen(Qt::white);
        painter.drawText(fallback.rect(), Qt::AlignCenter, 
                        QString("Failed to load\nURL image %1").arg(index + 1));
        processLoadedImage(index, fallback);
        return;
    }

    QByteArray imageData = reply->readAll();
    QImage image;
    if (!image.loadFromData(imageData)) {
        qDebug() << "Failed to load image from URL data";
        return;
    }

    // Cache the loaded image
    m_urlImageCache.insert(reply->url(), image);
    processLoadedImage(index, image);
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

    QString cacheKey = QString("text_%1").arg(text);
    QSGTexture* texture = TextureBuffer::instance().acquire(window(), cacheKey);
    
    if (!texture) {
        // Create new texture for text
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
        painter.setRenderHints(QPainter::TextAntialiasing);
        painter.drawText(textImage.rect(), Qt::AlignCenter, text);
        painter.end();
        
        texture = window()->createTextureFromImage(
            textImage,
            QQuickWindow::TextureHasAlphaChannel
        );
        
        if (texture) {
            // Store in TextureBuffer instead of m_textureCache
            TextureBuffer::instance().acquire(window(), cacheKey);
        }
    }
    
    return texture ? createTexturedRect(rect, texture) : nullptr;
}

QSGGeometryNode* CustomImageListView::createRowTitleNode(const QString &text, const QRectF &rect)
{
    if (!window()) {
        return nullptr;
    }

    // Ensure minimum dimensions
    int width = qMax(1, static_cast<int>(std::ceil(rect.width())));
    int height = qMax(1, static_cast<int>(std::ceil(rect.height())));

    // Create image with guaranteed valid dimensions
    QImage textImage(width, height, QImage::Format_ARGB32_Premultiplied);
    if (textImage.isNull()) {
        qWarning() << "Failed to create text image with size:" << width << "x" << height;
        return nullptr;
    }

    // Initialize image with transparent background
    textImage.fill(QColor(0, 0, 0, 0));

    // Create and configure painter
    QPainter painter;
    bool started = painter.begin(&textImage);
    if (!started) {
        qWarning() << "Failed to start painting on image";
        return nullptr;
    }

    // Configure text rendering
    static const QFont titleFont("Arial", 24, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    painter.setRenderHints(QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    // Draw text centered vertically
    QFontMetrics fm(titleFont);
    int yPos = (height + fm.ascent() - fm.descent()) / 2;
    painter.drawText(0, yPos, text);
    
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

    return createTexturedRect(rect, texture);
}

QSGNode *CustomImageListView::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (!window() || !window()->isSceneGraphInitialized()) {
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
    
    // Calculate layout
    m_itemsPerRow = qMax(1, int((width() + m_spacing) / (m_itemWidth + m_spacing)));
    int totalRows = (m_count + m_itemsPerRow - 1) / m_itemsPerRow;

    // Track current Y position
    qreal currentY = -m_contentY;

    // Create row containers
    for (int row = 0; row < totalRows; ++row) {
        // Add spacing between rows (except first row)
        if (row > 0) {
            currentY += m_rowSpacing;
        }

        // Add row title
        if (row < m_rowTitles.size()) {
            QRectF titleRect(0, currentY, width(), 40);
            QSGGeometryNode *titleNode = createRowTitleNode(m_rowTitles[row], titleRect);
            if (titleNode) {
                parentNode->appendChildNode(titleNode);
            }
            currentY += 40; // Add title height
        }

        // Calculate items for this row
        for (int col = 0; col < m_itemsPerRow; ++col) {
            int index = row * m_itemsPerRow + col;
            if (index >= m_count || !m_nodes.contains(index)) continue;

            qreal xPos = col * (m_itemWidth + m_spacing);
            QRectF rect(xPos - m_contentX, currentY, m_itemWidth, m_itemHeight);

            // Skip if completely outside visible area
            if (rect.bottom() < 0 || rect.top() > height()) continue;

            // Create container node for this item
            QSGNode* itemContainer = new QSGNode;
            
            // Add base image
            if (m_nodes[index].texture) {
                QSGGeometryNode *imageNode = createTexturedRect(rect, m_nodes[index].texture);
                itemContainer->appendChildNode(imageNode);
                m_nodes[index].node = imageNode;
            }

            // Add focus/hover effects if this is the current item
            if (index == m_currentIndex) {
                // Inner glow
                QSGGeometryNode *glowNode = new QSGGeometryNode;
                QSGGeometry *glowGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
                glowGeometry->setDrawingMode(GL_TRIANGLE_STRIP);
                
                QRectF glowRect = rect.adjusted(-2, -2, 2, 2);
                QSGGeometry::Point2D *glowVertices = glowGeometry->vertexDataAsPoint2D();
                glowVertices[0].set(glowRect.left(), glowRect.top());
                glowVertices[1].set(glowRect.right(), glowRect.top());
                glowVertices[2].set(glowRect.left(), glowRect.bottom());
                glowVertices[3].set(glowRect.right(), glowRect.bottom());
                
                glowNode->setGeometry(glowGeometry);
                glowNode->setFlag(QSGNode::OwnsGeometry);
                
                QSGFlatColorMaterial *glowMaterial = new QSGFlatColorMaterial;
                glowMaterial->setColor(QColor(0, 120, 215, hasActiveFocus() ? 180 : 100));
                glowNode->setMaterial(glowMaterial);
                glowNode->setFlag(QSGNode::OwnsMaterial);
                
                itemContainer->appendChildNode(glowNode);

                // Focus border
                QSGGeometryNode *focusNode = new QSGGeometryNode;
                QSGGeometry *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 5);
                geometry->setDrawingMode(GL_LINE_STRIP);
                geometry->setLineWidth(3);
                
                QSGGeometry::Point2D *vertices = geometry->vertexDataAsPoint2D();
                vertices[0].set(rect.left(), rect.top());
                vertices[1].set(rect.right(), rect.top());
                vertices[2].set(rect.right(), rect.bottom());
                vertices[3].set(rect.left(), rect.bottom());
                vertices[4].set(rect.left(), rect.top());
                
                focusNode->setGeometry(geometry);
                focusNode->setFlag(QSGNode::OwnsGeometry);
                
                QSGFlatColorMaterial *material = new QSGFlatColorMaterial;
                material->setColor(hasActiveFocus() ? QColor(0, 120, 215) : QColor(128, 128, 128));
                focusNode->setMaterial(material);
                focusNode->setFlag(QSGNode::OwnsMaterial);
                
                itemContainer->appendChildNode(focusNode);
            }

            // Add title overlay
            if (index < m_imageTitles.size()) {
                // Create semi-transparent background for text
                QSGGeometryNode *textBgNode = new QSGGeometryNode;
                QSGGeometry *bgGeometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
                bgGeometry->setDrawingMode(GL_TRIANGLE_STRIP);
                
                QRectF textRect(rect.left(), rect.bottom() - 40, rect.width(), 40);
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
                
                itemContainer->appendChildNode(textBgNode);

                // Create text using optimized texture atlas
                QString title = m_imageTitles[index];
                QSGGeometryNode *textNode = createOptimizedTextNode(title, textRect);
                if (textNode) {
                    itemContainer->appendChildNode(textNode);
                }
            }

            parentNode->appendChildNode(itemContainer);
        }

        currentY += m_itemHeight; // Add row height
    }
    
    return parentNode;
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
    if (m_currentIndex > 0) {
        setCurrentIndex(m_currentIndex - 1);
        ensureFocus();
        update();
    }
}

void CustomImageListView::navigateRight()
{
    if (m_currentIndex < m_count - 1) {
        setCurrentIndex(m_currentIndex + 1);
        ensureFocus();
        update();
    }
}

void CustomImageListView::navigateUp()
{
    int newIndex = m_currentIndex - m_itemsPerRow;
    if (newIndex >= 0) {
        setCurrentIndex(newIndex);
        ensureIndexVisible(newIndex);
        ensureFocus();
        update();
    }
}

void CustomImageListView::navigateDown()
{
    int newIndex = m_currentIndex + m_itemsPerRow;
    if (newIndex < m_count) {
        setCurrentIndex(newIndex);
        ensureIndexVisible(newIndex);
        ensureFocus();
        update();
    }
}

void CustomImageListView::ensureIndexVisible(int index)
{
    int row = getRowFromIndex(index);
    int col = getColumnFromIndex(index);
    
    qreal itemY = row * (m_itemHeight + m_rowSpacing) + 50;  // Account for title
    qreal itemX = col * (m_itemWidth + m_spacing);
    
    // Adjust vertical scroll
    if (itemY < m_contentY) {
        setContentY(itemY - m_rowSpacing);  // Show a bit of the previous row
    } else if ((itemY + m_itemHeight) > (m_contentY + height())) {
        setContentY(itemY + m_itemHeight - height() + m_rowSpacing);
    }
    
    // Adjust horizontal scroll
    if (itemX < m_contentX) {
        setContentX(itemX - m_spacing);
    } else if ((itemX + m_itemWidth) > (m_contentX + width())) {
        setContentX(itemX + m_itemWidth - width() + m_spacing);
    }
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

qreal CustomImageListView::contentHeight() const
{
    int totalRows = (m_count + m_itemsPerRow - 1) / m_itemsPerRow;
    qreal height = 0;
    
    for (int row = 0; row < totalRows; ++row) {
        if (row > 0) {
            height += m_rowSpacing;
        }
        if (row < m_rowTitles.size()) {
            height += 40; // Title height
        }
        height += m_itemHeight;
    }
    
    return height;
}

void CustomImageListView::wheelEvent(QWheelEvent *event)
{
    // Vertical scrolling
    if (event->angleDelta().y() != 0) {
        qreal delta = event->angleDelta().y() / 120.0 * 50.0;
        qreal newY = m_contentY - delta;
        newY = qBound(0.0, newY, qMax(0.0, contentHeight() - height()));
        setContentY(newY);
    }
    
    // Horizontal scrolling
    if (event->angleDelta().x() != 0) {
        qreal delta = event->angleDelta().x() / 120.0 * 50.0;
        qreal newX = m_contentX - delta;
        newX = qBound(0.0, newX, qMax(0.0, contentWidth() - width()));
        setContentX(newX);
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

