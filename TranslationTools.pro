QT += quick concurrent widgets xlsx xml

CONFIG += c++17

DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += \
    src/translateworker.h

SOURCES += \
        main.cpp \
        src/translateworker.cpp

RESOURCES += qml.qrc

TRANSLATIONS += \
    zh_CN.ts \
    en_US.ts \
    fr_FR.ts

# Additional import path used to resolve QML modules in Qt Creator's code model

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
