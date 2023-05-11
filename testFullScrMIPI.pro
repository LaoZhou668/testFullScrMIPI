#-------------------------------------------------
#
# Project created by QtCreator 2023-03-30T14:20:06
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = testFullScrMIPI
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    mainproc.cpp \
    postprocess.cpp

HEADERS += \
        mainwindow.h \
    mainproc.h \
    postprocess.h \
    rknn_api.h

FORMS += \
        mainwindow.ui


INCLUDEPATH += /opt/testopencv/include \
               /opt/testopencv/include/opencv \
               /opt/testopencv/include/opencv2 \

LIBS += /opt/testopencv/lib/libopencv_core.so    \
        /opt/testopencv/lib/libopencv_imgproc.so \
        /opt/testopencv/lib/libopencv_imgcodecs.so \
        /opt/testopencv/lib/libopencv_videoio.so \
        /opt/testopencv/lib/libopencv_features2d.so \
        /usr/lib/librknnrt.so
