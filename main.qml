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

        onLinkActivated: function(action, jsonData) {
            // Parse the JSON string into an object
            let data = JSON.parse(jsonData)
            console.log("Link activated:", 
                "\nAction:", action,
                "\nTitle:", data.title,
                "\nURL:", data.url,
                "\nDescription:", data.description,
                "\nCategory:", data.category,
                "\nID:", data.id,
                "\nThumbnail:", data.thumbnailUrl,
                "\nMood Image:", data.moodImageUri,
                "\nLinks:", JSON.stringify(data.links, null, 2)
            )
            
            switch(action) {
                case "OK":
                    // Handle playback with complete data
                    console.log("Playing content:", data.title)
                    console.log("Using URL:", data.links.play || data.url)
                    break
                    
                case "info":
                    // Handle info display with complete data
                    console.log("Showing info for:", data.title)
                    console.log("Using URL:", data.links.info)
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
