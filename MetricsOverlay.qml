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
            text: "Scene Graph Nodes: 0"
        }
        
        Text {
            id: textureCountText
            color: "#ffffff"
            font.pixelSize: 12
            text: "Active Textures: 0"
        }
    }
    
    Timer {
        interval: updateInterval
        running: root.visible
        repeat: true
        onTriggered: {
            // Update metrics display
            var nodes = imageListView ? imageListView.nodeCount : 0
            var textures = imageListView ? imageListView.textureCount : 0
            nodeCountText.text = "Scene Graph Nodes: " + nodes
            textureCountText.text = "Active Textures: " + textures
        }
    }

    Component.onCompleted: {
        console.log("MetricsOverlay initialized")
    }
}
