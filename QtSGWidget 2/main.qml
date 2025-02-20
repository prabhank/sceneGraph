import QtQuick 2.5
import QtQuick.Window 2.2
import Custom 1.0

Window {
    visible: true
    width: 1280
    height: 720
    color: "black"
    title: qsTr("Menu Gallery View")

    CustomImageListView {
        id: imageListView
        anchors.fill: parent
        jsonSource: "qrc:/data/embeddedHubMenu.json"  // Fixed filename from Hyb to Hub
        focus: true
        clip: true
    }
}
