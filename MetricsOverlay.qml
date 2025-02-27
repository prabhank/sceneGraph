import QtQuick 2.5

Rectangle {
    id: root
    color: Qt.rgba(0, 0, 0, 0.6)
    radius: 4
    border.color: "#404040"
    border.width: 1

    // Properties
    property var imageListView: null
    property bool enabled: true
    property int updateInterval: 1000
    width: metrics.width + 20
    height: metrics.height + 20
    
    visible: enabled && imageListView !== null
    
    Column {
        id: metrics
        anchors.centerIn: parent
        spacing: 4
        
        Text {
            id: nodeCountText
            color: "#ffffff"
            font.pixelSize: 12
            visible: false  // Initially hidden, Timer will control visibility
            text: "Scene Graph Nodes: 0"  // Default text, Timer will update
        }
        
        Text {
            id: textureCountText
            color: "#ffffff"
            font.pixelSize: 12
            visible: false  // Initially hidden, Timer will control visibility
            text: "Active Textures: 0"  // Default text, Timer will update
        }
        
        Text {
            id: controlText
            color: "#808080"
            font.pixelSize: 10
            text: "Press 'N' for nodes\nPress 'T' for textures"
        }
    }
    
    Timer {
        interval: updateInterval
        running: root.visible && imageListView !== null
        repeat: true
        onTriggered: {
            if (imageListView) {
                // Update node metrics
                nodeCountText.visible = imageListView.enableNodeMetrics
                if (imageListView.enableNodeMetrics) {
                    nodeCountText.text = "Scene Graph Nodes: " + imageListView.nodeCount
                }
                
                // Update texture metrics
                textureCountText.visible = imageListView.enableTextureMetrics
                if (imageListView.enableTextureMetrics) {
                    textureCountText.text = "Active Textures: " + imageListView.textureCount
                }
            }
        }
    }

    // Add key handling for individual metric toggles
    Keys.onPressed: {
        if (event.key === Qt.Key_N && imageListView) {
            imageListView.enableNodeMetrics = !imageListView.enableNodeMetrics
            event.accepted = true
        } else if (event.key === Qt.Key_T && imageListView) {
            imageListView.enableTextureMetrics = !imageListView.enableTextureMetrics
            event.accepted = true
        }
    }

    Component.onCompleted: {
        console.log("MetricsOverlay initialized")
    }
}
