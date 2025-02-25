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
        jsonSource: "qrc:/data/embeddedHubMenu.json"
        focus: true
        clip: true
        startPositionX: 50  // Specify the starting X position here (e.g., 50 pixels)

        onLinkActivated: function(action, url) {
            console.log("Link activated - Action:", action, "URL:", url)
            
            switch(action) {
                case "OK":
                    console.log("Processing OK action with URL:", url)
                    // Add your content playback logic here
                    break
                    
                case "info":
                    console.log("Processing info action with URL:", url)
                    // Add your info display logic here
                    break
                    
                default:
                    console.log("Unknown action type:", action)
                    break
            }
        }

        onMoodImageSelected: function(url) {
            console.log("Mood image URL:", url)
            // Handle the mood image URL here
        }
        
        // Add focus handling
        onFocusChanged: {
            if (focus) {
                console.log("CustomImageListView gained focus")
            } else {
                console.log("CustomImageListView lost focus")
            }
        }

        onAssetFocused: function(assetData) {
            console.log("Asset focused, complete data:", JSON.stringify(assetData, null, 2))
            // Now you have access to all JSON fields exactly as they were in the source
        }
    }
}
