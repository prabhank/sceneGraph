#include "verify_resources.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QImage>

bool ResourceVerifier::verifyImages()
{
    QStringList imagePaths = {
        ":/images/img1.jpg",
        ":/images/img2.jpg",
        ":/images/img3.jpg",
        ":/images/img4.jpg",
        ":/images/img5.jpg"
    };

    qDebug() << "\n=== Verifying Resource Images ===";
    
    bool allValid = true;
    for (const QString &path : imagePaths) {
        if (!verifyImage(path)) {
            allValid = false;
        }
    }
    
    return allValid;
}

bool ResourceVerifier::verifyImage(const QString &path)
{
    qDebug() << "\nVerifying image:" << path;
    
    QFile file(path);
    if (!file.exists()) {
        qDebug() << "ERROR: File does not exist:" << path;
        return false;
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "ERROR: Cannot open file:" << file.errorString();
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    if (data.isEmpty()) {
        qDebug() << "ERROR: File is empty:" << path;
        return false;
    }
    
    qDebug() << "File size:" << data.size() << "bytes";
    
    QImage image;
    if (!image.loadFromData(data)) {
        qDebug() << "ERROR: Cannot load image data from:" << path;
        return false;
    }
    
    debugImageInfo(path);
    return true;
}

void ResourceVerifier::debugImageInfo(const QString &path)
{
    QImage image(path);
    if (!image.isNull()) {
        qDebug() << "SUCCESS: Image verified";
        qDebug() << "- Path:" << path;
        qDebug() << "- Size:" << image.size();
        qDebug() << "- Format:" << image.format();
        qDebug() << "- Depth:" << image.depth();
        qDebug() << "- Has alpha:" << image.hasAlphaChannel();
    }
}

bool verifyResources()
{
    // Minimal checks
    QStringList resourcePaths = {
        ":/images/img1.jpg",
        ":/images/img2.jpg",
        ":/images/img3.jpg",
        ":/images/img4.jpg",
        ":/images/img5.jpg",
        ":/main.qml"
    };

    bool allExist = true;
    for (const QString &path : resourcePaths) {
        QFile f(path);
        if (!f.exists()) {
            qWarning() << "Missing resource:" << path;
            allExist = false;
        }
    }
    return allExist;
}
