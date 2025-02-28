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

        // Add vertical menu rectangle
        Rectangle {
            id: verticalMenu
            width: 60
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            color: "#333333"
            activeFocusOnTab: true
            focus: true // Take initial focus
            
            // Add visual indicator for focus
            Rectangle {
                id: focusIndicator
                width: 4
                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                }
                color: parent.activeFocus ? "#00ff00" : "transparent"
            }

            // Add KeyNavigation instead of Keys handlers
            KeyNavigation.right: viewLoader.item || launchButton
            
            // Make it focusable
            MouseArea {
                anchors.fill: parent
                onClicked: verticalMenu.forceActiveFocus()
            }

            // Add state for better focus visibility
            states: [
                State {
                    name: "focused"
                    when: verticalMenu.activeFocus
                    PropertyChanges {
                        target: verticalMenu
                        color: "#444444"
                    }
                }
            ]
            
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }

        // Key handler for root focus scope
        Keys.onPressed: {
            console.log("Root focus scope received key:", event.key)
            if (event.key === Qt.Key_Space) {
            console.log("Space key pressed in root focus scope")
            // Add any additional logic for space key here
            event.accepted = true
            } else if (isBackKey(event.key)) {
            if (viewLoader.sourceComponent) {
                viewLoader.sourceComponent = null
                event.accepted = true
                console.log("Gallery unloaded due to back key in root focus scope")
            }
            }
        }



        // Loader for dynamic loading
        Loader {
            id: viewLoader
            anchors {
                left: verticalMenu.right
                right: parent.right
                top: parent.top
                bottom: parent.bottom
            }
            focus: true  // Loader should have focus within the scope

            onLoaded: {
                console.log("Loader onLoaded called")
                if (item) {
                    // Set up KeyNavigation for loaded item
                    item.KeyNavigation.left = verticalMenu
                    focusTimer.start()
                }
            }

            onStatusChanged: {
                console.log("Loader status:", status)
                if (status === Loader.Null) {
                    rootFocusScope.focus = true
                    console.log("Loader unloaded, focus returned to root scope")
                }
            }

            // Add Timer to force focus after loading
            Timer {
                id: focusTimer
                interval: 100  // Short delay to ensure component is ready
                repeat: false
                onTriggered: {
                    console.log("Focus timer triggered")
                    if (viewLoader.item) {
                        viewLoader.item.forceActiveFocus()
                        console.log("Force focus given to gallery")
                    }
                }
            }
        }

        // Add a debug connection to track focus changes
        Connections {
            target: verticalMenu
            onActiveFocusChanged: console.log("Vertical menu focus changed:", verticalMenu.activeFocus)
        }

        // Launch button
        Button {
            id: launchButton
            anchors.centerIn: viewLoader
            text: "Launch Gallery"
            visible: !viewLoader.sourceComponent
            onClicked: {
            viewLoader.sourceComponent = galleryComponent
            }
            // Add KeyNavigation
            KeyNavigation.left: verticalMenu
        }


        // Component definition for the gallery view
        Component {
            id: galleryComponent
            
            CustomImageListView {
                anchors.fill: parent
                jsonSource: "qrc:/data/embeddedHubMenu.json"
                focus: true
                clip: true
                startPositionX: 50

                // Add KeyNavigation
                KeyNavigation.left: verticalMenu

                // Only handle specific keys, let others propagate
                Keys.onPressed: {
                    console.log("Gallery received key:", event.key)
                    if (event.key === Qt.Key_M) {
                        metricsOverlay.enabled = !metricsOverlay.enabled
                        event.accepted = true
                    } else if (event.key === Qt.Key_N) {
                        enableNodeMetrics = !enableNodeMetrics
                        event.accepted = true
                    } else if (event.key === Qt.Key_T) {
                        enableTextureMetrics = !enableTextureMetrics
                        event.accepted = true
                    }
                    // Don't accept other keys, let them propagate
                }

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

                onAssetFocused: function(assetData) {
                    console.log("Asset focused, complete data:", JSON.stringify(assetData, null, 2))
                    // Now you have access to all JSON fields exactly as they were in the source
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

                // Set the KeyNavigation target explicitly
                property Item navigationLeftTarget: verticalMenu

                // Focus handling
                onActiveFocusChanged: {
                    console.log("Gallery active focus changed:", activeFocus)
                    if (!activeFocus) {
                        console.log("Focus candidate:", verticalMenu.activeFocus ? "vertical menu" : "other")
                    }
                }

                Component.onCompleted: {
                    console.log("Gallery component completed")
                    forceActiveFocus()
                }
            }
        }
    }

    // Window level key handling
    Keys.onPressed: {
        console.log("Window received key:", event.key, "Active focus item:", Window.activeFocusItem)
        if (isBackKey(event.key)) {
            if (viewLoader.sourceComponent) {
                viewLoader.sourceComponent = null
                event.accepted = true
                console.log("Gallery unloaded due to back key")
            }
        }
    }


    // Metrics overlay
    MetricsOverlay {
        id: metricsOverlay
        imageListView: viewLoader.item
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        enabled: viewLoader.item ? true : false
        z: 1000
        visible: viewLoader.item ? true : false
    }
}
