QT += core gui quick
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtSGWidget
TEMPLATE = app

CONFIG += c++11

# Optimize for embedded systems
QMAKE_CXXFLAGS += -O2
CONFIG += optimize_full

# Source files
SOURCES += \
    main.cpp \
    customrectangle.cpp \
    customlistview.cpp \
    customimagelistview.cpp \
    verify_resources.cpp \
    texturemanager.cpp \
    texturebuffer.cpp \
    # ... existing sources ...

HEADERS += \
    customrectangle.h \
    customlistview.h \
    customimagelistview.h \
    verify_resources.h \
    texturemanager.h \
    texturebuffer.h \
    # ... existing headers ...

RESOURCES += qml.qrc \
    resources.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Enable OpenGL ES 2.0 on embedded platforms
android|ios|qnx {
    DEFINES += QT_OPENGL_ES_2
}

# For Raspberry Pi specific optimizations
linux-rasp-pi-* {
    DEFINES += RASPBERRY_PI
    QMAKE_CXXFLAGS += -mfpu=neon-vfpv4 -mfloat-abi=hard
}

# Default rules for deployment.
include(deployment.pri)
