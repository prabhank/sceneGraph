import QtQuick 2.6

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
    
    // Metrics display
    Column {
        id: metrics
        anchors.centerIn: parent
        spacing: 4
        
        Text {
            color: "#ffffff"
            font.pixelSize: 12
            text: "Nodes: " + (imageListView ? imageListView.nodeCount : 0)
        }
        
        Text {
            color: "#ffffff"
            font.pixelSize: 12
            text: "Textures: " + (imageListView ? imageListView.textureCount : 0)
        }
    }
    
    // Polling timer for updates
    Timer {
        interval: updateInterval
        running: root.visible
        repeat: true
        onTriggered: {
            // Force refresh via geometry change
            var w = root.width
            root.width = w - 1
            root.width = w
        }
    }
    
    // Drag support
    MouseArea {
        anchors.fill: parent
        drag.target: parent
        
        // Constrain to parent boundaries
        drag.minimumX: 0
        drag.minimumY: 0
        drag.maximumX: parent.parent ? parent.parent.width - parent.width : 0
        drag.maximumY: parent.parent ? parent.parent.height - parent.height : 0
    }
}
