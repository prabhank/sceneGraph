#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QDebug>
#include <QDir>
#include <QSurfaceFormat>
#include <QQuickWindow>  // Add this include
#include <QSGRendererInterface>  // Add this include
#include "customrectangle.h"
#include "customlistview.h"
#include "customimagelistview.h"
#include "verify_resources.h"

int main(int argc, char *argv[])
{
    // Enable OpenGL debug output
    qputenv("QSG_INFO", "1");
    
    // Force OpenGL on desktop platforms
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    
    // Set rendering backend
    QQuickWindow::setSceneGraphBackend(QSGRendererInterface::OpenGL);
    
    // Configure surface format for better compatibility
    QSurfaceFormat format;
    format.setVersion(2, 1);  // OpenGL 2.1 for better compatibility
    format.setProfile(QSurfaceFormat::NoProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(0);  // Disable MSAA for better performance
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);

    // Initialize resources
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(qml);

    // Initialize resources for images and data
    Q_INIT_RESOURCE(resources);
    
    // Verify resources are loaded correctly
    ResourceVerifier::verifyResources();
    ResourceVerifier::verifyImages();

    // Register types
    qmlRegisterType<CustomRectangle>("Custom", 1, 0, "CustomRectangle");
    qmlRegisterType<CustomListView>("Custom", 1, 0, "CustomListView");
    qmlRegisterType<CustomImageListView>("Custom", 1, 0, "CustomImageListView");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
