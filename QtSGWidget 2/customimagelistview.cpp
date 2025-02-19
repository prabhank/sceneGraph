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
//#include <QSGRendererInterface>

CustomImageListView::CustomImageListView(QQuickItem *parent)
    : QQuickItem(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    // Enable OpenGL debug output
    qputenv("QSG_INFO", "1");
    
    // Force OpenGL on desktop platforms
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    
    // Set rendering backend
    //QQuickWindow::setSceneGraphBackend(QSGRendererInterface::OpenGL);
    
    // Configure surface format for better compatibility
    QSurfaceFormat format;
    format.setVersion(2, 1);  // OpenGL 2.1 for better compatibility
    format.setProfile(QSurfaceFormat::NoProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(0);  // Disable MSAA for better performance
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);

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
            if (!m_windowReady) {
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
    
    return true;
}

void CustomImageListView::loadImage(int index)
{
    if (m_isDestroying || !ensureValidWindow() || index >= m_imageData.size()) {
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

    // Load from URL
    const ImageData &imgData = m_imageData[index];
    QUrl url(imgData.url);
    
    if (url.isValid()) {
        loadUrlImage(index, url);
    } else {
        createFallbackTexture(index);
    }

    m_isLoading = false;
}

bool CustomImageListView::ensureValidWindow() const
{
    return window() && m_windowReady && !m_isDestroying;
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

void CustomImageListView::loadUrlImage(int index, const QUrl &url)
{
    QString urlStr = url.toString();
    
    // Handle resource URLs
    if (url.scheme() == "qrc") {
        // Convert qrc:/path to :/path format
        urlStr = ":" + url.path();
        QImage image = loadLocalImageFromPath(urlStr);
        if (!image.isNull()) {
            processLoadedImage(index, image);
            return;
        }
    }
    
    // Check cache first
    if (m_urlImageCache.contains(url)) {
        QImage image = m_urlImageCache[url];
        processLoadedImage(index, image);
        return;
    }

    // For network URLs, proceed with network request
    if (url.scheme().startsWith("http")) {
        // Cancel any existing request for this index
        if (m_pendingRequests.contains(index)) {
            m_pendingRequests[index]->abort();
            m_pendingRequests[index]->deleteLater();
            m_pendingRequests.remove(index);
        }

        QNetworkRequest request(url);
        QNetworkReply *reply = m_networkManager->get(request);
        m_pendingRequests[index] = reply;

        connect(reply, &QNetworkReply::finished, this, [this, reply, index]() {
            handleNetworkReply(reply, index);
        });
    }
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

    // Create image with guaranteed valid dimensions
    int width = qMax(1, static_cast<int>(std::ceil(rect.width())));
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
    static const QFont titleFont("Arial", 24, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    painter.setRenderHints(QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    // Draw text with left alignment and vertical centering
    QFontMetrics fm(titleFont);
    int yPos = (height + fm.ascent() - fm.descent()) / 2;
    painter.drawText(10, yPos, text); // Add 10px left margin
    
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
    if (!window() || !m_windowReady) {
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
    
    // Track current Y position and image index
    qreal currentY = -m_contentY;
    int currentImageIndex = 0;
    
    // Create category containers
    for (int categoryIndex = 0; categoryIndex < m_rowTitles.size(); ++categoryIndex) {
        // Add spacing between categories (except first)
        if (categoryIndex > 0) {
            currentY += m_rowSpacing;
        }

        // Add category title
        QString categoryName = m_rowTitles[categoryIndex];
        QRectF titleRect(0, currentY, width(), 40);
        QSGGeometryNode *titleNode = createRowTitleNode(categoryName, titleRect);
        if (titleNode) {
            parentNode->appendChildNode(titleNode);
        }
        currentY += 40; // Add title height

        // Count images in this category
        int categoryImageCount = 0;
        for (const ImageData &imgData : m_imageData) {
            if (imgData.category == categoryName) {
                categoryImageCount++;
            }
        }

        // Calculate rows needed for this category
        int rowsInCategory = (categoryImageCount + m_itemsPerRow - 1) / m_itemsPerRow;
        
        // Create rows for this category
        int imagesInCategory = 0;
        for (int row = 0; row < rowsInCategory && currentImageIndex < m_count; ++row) {
            for (int col = 0; col < m_itemsPerRow && imagesInCategory < categoryImageCount; ++col) {
                if (!m_nodes.contains(currentImageIndex)) {
                    currentImageIndex++;
                    imagesInCategory++;
                    continue;
                }

                qreal xPos = col * (m_itemWidth + m_spacing);
                QRectF rect(xPos - m_contentX, currentY, m_itemWidth, m_itemHeight);

                // Skip if outside visible area
                if (rect.bottom() < 0 || rect.top() > height()) {
                    currentImageIndex++;
                    imagesInCategory++;
                    continue;
                }

                // Create item container and add image
                QSGNode* itemContainer = new QSGNode;
                if (m_nodes[currentImageIndex].texture) {
                    QSGGeometryNode *imageNode = createTexturedRect(rect, m_nodes[currentImageIndex].texture);
                    itemContainer->appendChildNode(imageNode);
                    m_nodes[currentImageIndex].node = imageNode;
                }

                // Add selection/focus effects
                if (currentImageIndex == m_currentIndex) {
                    addSelectionEffects(itemContainer, rect);
                }

                // Add title overlay
                if (currentImageIndex < m_imageData.size()) {
                    addTitleOverlay(itemContainer, rect, m_imageData[currentImageIndex].title);
                }

                parentNode->appendChildNode(itemContainer);
                currentImageIndex++;
                imagesInCategory++;
            }
            currentY += m_itemHeight + m_rowSpacing;
        }
    }
    
    return parentNode;
}

// Helper method for selection effects
void CustomImageListView::addSelectionEffects(QSGNode* container, const QRectF& rect)
{
    // Add glow effect
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
    
    container->appendChildNode(glowNode);
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
    // Get current category and position info
    QString currentCategory = m_imageData[m_currentIndex].category;
    int categoryIndex = m_rowTitles.indexOf(currentCategory);
    
    // Can't go up from first category
    if (categoryIndex <= 0) {
        return;
    }

    // Calculate current column in current row
    int currentCol = 0;
    int itemsInCategory = 0;
    
    // Count items until current index to determine column
    for (int i = 0; i < m_imageData.size(); i++) {
        if (m_imageData[i].category == currentCategory) {
            if (i == m_currentIndex) {
                currentCol = itemsInCategory % m_itemsPerRow;
                break;
            }
            itemsInCategory++;
        }
    }

    // Get target category
    QString targetCategory = m_rowTitles[categoryIndex - 1];
    
    // Find matching column position in previous category
    int targetIndex = -1;
    int col = 0;
    int lastIndexInTargetCategory = -1;
    
    // Find first item with matching column or last item in category
    for (int i = 0; i < m_imageData.size(); i++) {
        if (m_imageData[i].category == targetCategory) {
            if (col == currentCol) {
                targetIndex = i;
                break;
            }
            col = (col + 1) % m_itemsPerRow;
            lastIndexInTargetCategory = i;
        } else if (targetIndex >= 0) {
            break; // Stop after target category
        }
    }

    // If no matching column found, use last item in target category
    if (targetIndex == -1 && lastIndexInTargetCategory >= 0) {
        targetIndex = lastIndexInTargetCategory;
    }

    if (targetIndex >= 0) {
        setCurrentIndex(targetIndex);
        ensureIndexVisible(targetIndex);
        ensureFocus();
        update();
    }
}

void CustomImageListView::navigateDown()
{
    // Get current category and position info
    QString currentCategory = m_imageData[m_currentIndex].category;
    int categoryIndex = m_rowTitles.indexOf(currentCategory);
    
    // Can't go down from last category
    if (categoryIndex >= m_rowTitles.size() - 1) {
        return;
    }

    // Calculate current column in current row
    int currentCol = 0;
    int itemsInCategory = 0;
    
    // Count items until current index to determine column
    for (int i = 0; i < m_imageData.size(); i++) {
        if (m_imageData[i].category == currentCategory) {
            if (i == m_currentIndex) {
                currentCol = itemsInCategory % m_itemsPerRow;
                break;
            }
            itemsInCategory++;
        }
    }

    // Get target category
    QString targetCategory = m_rowTitles[categoryIndex + 1];
    
    // Find matching column position in next category
    int targetIndex = -1;
    int col = 0;
    
    // Find first item with matching column
    for (int i = 0; i < m_imageData.size(); i++) {
        if (m_imageData[i].category == targetCategory) {
            if (col == currentCol) {
                targetIndex = i;
                break;
            }
            col = (col + 1) % m_itemsPerRow;
        }
    }

    // If no matching column found, use first item in target category
    if (targetIndex == -1) {
        for (int i = 0; i < m_imageData.size(); i++) {
            if (m_imageData[i].category == targetCategory) {
                targetIndex = i;
                break;
            }
        }
    }

    if (targetIndex >= 0) {
        setCurrentIndex(targetIndex);
        ensureIndexVisible(targetIndex);
        ensureFocus();
        update();
    }
}

void CustomImageListView::ensureIndexVisible(int index)
{
    if (index < 0 || index >= m_imageData.size()) {
        return;
    }

    // Find category for this index
    QString targetCategory = m_imageData[index].category;
    int categoryIndex = m_rowTitles.indexOf(targetCategory);
    
    qreal yOffset = m_rowSpacing;  // Initial padding
    
    // Calculate Y position for target category
    for (int i = 0; i < categoryIndex; ++i) {
        yOffset += m_rowSpacing * 2;  // Spacing between categories
        yOffset += 40;  // Category title

        // Count images in previous category
        int prevCategoryImages = 0;
        QString prevCategory = m_rowTitles[i];
        for (const ImageData &imgData : m_imageData) {
            if (imgData.category == prevCategory) {
                prevCategoryImages++;
            }
        }

        // Add height for previous category
        int rows = (prevCategoryImages + m_itemsPerRow - 1) / m_itemsPerRow;
        yOffset += rows * m_itemHeight;
        if (rows > 1) {
            yOffset += (rows - 1) * m_rowSpacing;
        }
    }

    // Add current category title
    yOffset += 40;

    // Find position within category
    int categoryPosition = 0;
    int targetPosition = 0;
    for (int i = 0; i < m_imageData.size(); i++) {
        if (m_imageData[i].category == targetCategory) {
            if (i == index) {
                targetPosition = categoryPosition;
                break;
            }
            categoryPosition++;
        }
    }

    // Calculate row and column
    int row = targetPosition / m_itemsPerRow;
    int col = targetPosition % m_itemsPerRow;

    // Add height for rows in current category
    yOffset += row * (m_itemHeight + m_rowSpacing);

    // Calculate x position
    qreal xOffset = col * (m_itemWidth + m_spacing);

    // Center the item in view
    qreal targetY = yOffset - (height() - m_itemHeight) / 2;
    qreal targetX = xOffset - (width() - m_itemWidth) / 2;

    // Bound the scroll positions
    targetY = qBound(0.0, targetY, qMax(0.0, contentHeight() - height()));
    targetX = qBound(0.0, targetX, qMax(0.0, contentWidth() - width()));

    setContentY(targetY);
    setContentX(targetX);
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
    qreal totalHeight = 0;
    
    // Add initial padding
    totalHeight += m_rowSpacing;
    
    for (int categoryIndex = 0; categoryIndex < m_rowTitles.size(); ++categoryIndex) {
        // Add spacing between categories
        if (categoryIndex > 0) {
            totalHeight += m_rowSpacing * 2;  // Double spacing between categories
        }

        // Add category title height
        totalHeight += 40;  // Title height

        // Count images in this category
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
        
        // Add spacing between rows within category
        if (rowsInCategory > 1) {
            totalHeight += (rowsInCategory - 1) * m_rowSpacing;
        }
    }

    // Add extra padding at bottom
    totalHeight += m_rowSpacing * 2;
    
    return totalHeight;
}

void CustomImageListView::wheelEvent(QWheelEvent *event)
{
    // Calculate smooth scrolling delta
    qreal verticalDelta = event->angleDelta().y() / 120.0 * m_itemHeight;
    qreal horizontalDelta = event->angleDelta().x() / 120.0 * m_itemWidth;
    
    if (event->modifiers() & Qt::ShiftModifier) {
        // Horizontal scrolling with Shift key
        qreal newX = m_contentX - horizontalDelta;
        newX = qBound(0.0, newX, qMax(0.0, contentWidth() - width()));
        setContentX(newX);
    } else {
        // Vertical scrolling
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
    if (source.scheme() == "qrc") {
        // Load from resources
        QString path = ":" + source.path();
        QFile file(path);
        
        if (!file.open(QIODevice::ReadOnly)) {
            // Try alternative path without leading colon
            QString altPath = source.path();
            if (altPath.startsWith("/")) {
                altPath = altPath.mid(1);
            }
            altPath = ":" + altPath;
            
            QFile altFile(altPath);
            if (altFile.open(QIODevice::ReadOnly)) {
                processJsonData(altFile.readAll());
                altFile.close();
            } else {
                qWarning() << "Failed to load JSON from both paths:";
                qWarning() << "Primary path:" << path;
                qWarning() << "Alternative path:" << altPath;
            }
        } else {
            processJsonData(file.readAll());
            file.close();
        }
    } else {
        // Network loading
        QNetworkRequest request(source);
        QNetworkReply *reply = m_networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                processJsonData(reply->readAll());
            } else {
                qWarning() << "Failed to load JSON:" << reply->errorString();
            }
            reply->deleteLater();
        });
    }
}

void CustomImageListView::processJsonData(const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qWarning() << "Invalid JSON data";
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "JSON root should be an object";
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray categories = root["categories"].toArray();
    
    qDebug() << "\n=== Processing JSON Data ===";
    qDebug() << "Number of categories:" << categories.size();
    
    m_imageData.clear();
    m_rowTitles.clear();
    
    int totalImages = 0;
    for (const QJsonValue &categoryVal : categories) {
        QJsonObject category = categoryVal.toObject();
        QString categoryName = category["name"].toString();
        m_rowTitles.append(categoryName);
        
        QJsonArray images = category["images"].toArray();
        qDebug() << "Category:" << categoryName << "Images count:" << images.size();
        
        for (const QJsonValue &imageVal : images) {
            QJsonObject image = imageVal.toObject();
            ImageData imgData;
            imgData.url = image["url"].toString();
            imgData.title = image["title"].toString();
            imgData.category = categoryName;
            m_imageData.append(imgData);
            totalImages++;
        }
    }

    // Update the view
    m_count = m_imageData.size();
    qDebug() << "Total images loaded:" << totalImages;
    
    // Force layout update
    m_itemsPerRow = qMax(1, int((width() + m_spacing) / (m_itemWidth + m_spacing)));
    
    // Clean existing nodes before loading new ones
    safeReleaseTextures();
    
    loadAllImages();
    emit countChanged();
    emit rowTitlesChanged();
    update();
}

void CustomImageListView::initializeGL()
{
    if (!window()) return;

    // Connect to window's sceneGraphInitialized signal
    connect(window(), &QQuickWindow::sceneGraphInitialized, this, [this]() {
        // Verify OpenGL context
        QOpenGLContext *context = window()->openglContext();
        if (context) {
            qDebug() << "OpenGL Version:" << context->format().majorVersion() 
                     << "." << context->format().minorVersion();
            qDebug() << "Using OpenGL:" << context->isValid();
        }
    }, Qt::DirectConnection);
}

