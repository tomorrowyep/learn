QT       += core gui
QT += multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    audioplayer.cpp \
    avframequeue.cpp \
    avpacketqueue.cpp \
    decodethread.cpp \
    demuxthread.cpp \
    main.cpp \
    videoplayer.cpp \
    widget.cpp

HEADERS += \
    audioplayer.h \
    avframequeue.h \
    avpacketqueue.h \
    avsync.h \
    decodethread.h \
    demuxthread.h \
    multimediaqueue.h \
    multimediathread.h \
    videoplayer.h \
    widget.h

# 设置包含路径、在 Windows 平台上指定库路径和库文件
INCLUDEPATH += $$PWD/libs/ffmpeglibs/include
win32:LIBS += -L$$PWD/libs/ffmpeglibs/lib -lavcodec -lavformat -lavutil -lswscale -lavdevice -lswresample

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc

DISTFILES +=