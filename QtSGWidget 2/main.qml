import QtQuick 2.5
import QtQuick.Window 2.2
import Custom 1.0

Window {
    visible: true
    width: 1280
    height: 720
    color: "black"  // Add this to see the window background
    title: qsTr("Custom Rectangle Example")

    CustomImageListView {
        id: imageListView
        anchors.fill: parent
        itemWidth: 200
        itemHeight: 200
        spacing: 20
        rowSpacing: 40
        focus: true
        clip: true
        
        // Update JSON source path
        jsonSource: "qrc:/data/images.json"
    }
}
