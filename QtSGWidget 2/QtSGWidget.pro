QT += core gui network quick
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtSGWidget
TEMPLATE = app

CONFIG += c++11

# Source files
SOURCES += \
    main.cpp \
    customrectangle.cpp \
    customlistview.cpp \
    customimagelistview.cpp \
    verify_resources.cpp \
    texturemanager.cpp \
    texturebuffer.cpp

HEADERS += \
    customrectangle.h \
    customlistview.h \
    customimagelistview.h \
    verify_resources.h \
    texturemanager.h \
    texturebuffer.h

# Resources
RESOURCES += \
    resources.qrc \
    qml.qrc

# Platform specific configurations
unix {
    # Try pkg-config first
    system(pkg-config --exists openssl) {
        message("Using pkg-config for OpenSSL")
        PKGCONFIG += openssl
    } else {
        # Check common OpenSSL locations
        exists(/usr/include/openssl/ssl.h) {
            LIBS += -L/usr/lib -lssl -lcrypto
        } else:exists(/usr/local/opt/openssl/include/openssl/ssl.h) {
            # macOS with Homebrew OpenSSL
            INCLUDEPATH += /usr/local/opt/openssl/include
            LIBS += -L/usr/local/opt/openssl/lib -lssl -lcrypto
        } else {
            # Disable SSL if not found
            message("OpenSSL not found - building without SSL support")
            DEFINES += QT_NO_SSL
        }
    }
}

# Raspberry Pi specific settings
linux-rasp-pi-* {
    DEFINES += RASPBERRY_PI
    QMAKE_CXXFLAGS += -mfpu=neon-vfpv4 -mfloat-abi=hard
    
    # Try to use system SSL on Raspberry Pi
    exists(/usr/include/openssl/ssl.h) {
        LIBS += -lssl -lcrypto
    } else {
        DEFINES += QT_NO_SSL
    }
}

# For Windows
win32 {
    LIBS += -lssleay32 -llibeay32
}

# Enable OpenGL ES 2.0 on embedded platforms
android|ios|qnx {
    DEFINES += QT_OPENGL_ES_2
}

# Default deployment rules
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Network configuration
DEFINES += QT_NETWORK_LIB
DEFINES += QT_NO_SSL=0
