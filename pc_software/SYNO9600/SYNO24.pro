QT       += core gui serialport axcontainer multimedia charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    amidite.cpp \
    circlewidget.cpp \
    delay.cpp \
    dmt.cpp \
    filemanager.cpp \
    function.cpp \
    killsequence.cpp \
    loghistory.cpp \
    main.cpp \
    progressbar_timer.cpp \
    qr_scanner.cpp \
    serial_custom.cpp \
    state_machine.cpp \
    syno24.cpp \
    tritry_collection.cpp \
    trityl.cpp \
    volume_manager.cpp

HEADERS += \
    amidite.h \
    circlewidget.h \
    delay.h \
    dmt.h \
    filemanager.h \
    function.h \
    killsequence.h \
    loghistory.h \
    macro.h \
    progressbar_timer.h \
    qr_scanner.h \
    serial_custom.h \
    state_machine.h \
    struct.h \
    syno24.h \
    tritry_collection.h \
    trityl.h \
    volume_manager.h

FORMS += \
    killsequence.ui \
    qr_scanner.ui \
    syno24.ui \
    tritry_collection.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc
RC_ICONS = SYNO96_ExtraLargeIcons256x256.ico
QXLSX_PARENTPATH=./         # current QXlsx path is . (. means curret directory)
QXLSX_HEADERPATH=./header/  # current QXlsx header path is ./header/
QXLSX_SOURCEPATH=./source/  # current QXlsx source path is ./source/
include(./QXlsx.pri)
