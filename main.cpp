#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCoreApplication>
#include "customrectangle.h"
#include "customlistview.h"
#include "customimagelistview.h"
#include "verify_resources.h"
#include <QLoggingCategory>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    //QLoggingCategory::setFilterRules("qt.scenegraph.general=true");
    
    // Initialize resources
    Q_INIT_RESOURCE(resources);
    Q_INIT_RESOURCE(qml);

    // Verify resources are loaded
    if (!ResourceVerifier::verifyResources()) {
        qWarning() << "Failed to verify resources!";
    }
    
    // Register types
    qmlRegisterType<CustomRectangle>("Custom", 1, 0, "CustomRectangle");
    qmlRegisterType<CustomListView>("Custom", 1, 0, "CustomListView");
    qmlRegisterType<CustomImageListView>("Custom", 1, 0, "CustomImageListView");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
