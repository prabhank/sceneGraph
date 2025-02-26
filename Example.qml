import QtQuick 2.6
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2

Item {
    id: root
    width: 1280
    height: 720
    
    CustomImageListView {
        id: imageListView
        anchors.fill: parent
        jsonSource: "qrc:/data/embeddedHubMenu.json"
        
        // Toggle metrics with M key
        Keys.onPressed: {
            if (event.key === Qt.Key_M) {
                metricsOverlay.enabled = !metricsOverlay.enabled
                event.accepted = true
            }
        }
    }
    
    // Add metrics overlay
    MetricsOverlay {
        id: metricsOverlay
        imageListView: imageListView
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        enabled: true
        z: 1000 // Ensure it's on top
    }
}
