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
        count: 15     // 3 rows x 5 images
        focus: true
        clip: true

        rowTitles: [
            "Action Movies",
            "Comedy",
            "Sci-Fi"
        ]
        
        localImageUrls: [
            // Row 1
            "images/img1.jpg", "images/img2.jpg", "images/img3.jpg", "images/img4.jpg", "images/img5.jpg",
            // Row 2
            "images/img1.jpg", "images/img2.jpg", "images/img3.jpg", "images/img4.jpg", "images/img5.jpg",
            // Row 3
            "images/img1.jpg", "images/img2.jpg", "images/img3.jpg", "images/img4.jpg", "images/img5.jpg"
        ]
        
        imageTitles: [
            // Row 1 titles
            "Movie 1", "Movie 2", "Movie 3", "Movie 4", "Movie 5",
            // Row 2 titles
            "Comedy 1", "Comedy 2", "Comedy 3", "Comedy 4", "Comedy 5",
            // Row 3 titles
            "SciFi 1", "SciFi 2", "SciFi 3", "SciFi 4", "SciFi 5"
        ]
    }
}
