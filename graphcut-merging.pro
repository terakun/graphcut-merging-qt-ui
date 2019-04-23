#-------------------------------------------------
#
# Project created by QtCreator 2019-04-22T17:52:28
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QMAKE_CXXFLAGS += -std=c++1z
QMAKE_CXXFLAGS_RELEASE += -Ofast -march=native
TARGET = graphcut-merging
TEMPLATE = app

DEPENDPATH += . /usr/local/include/opencv/ /usr/local/include/opencv2/
INCLUDEPATH += . /usr/local/include/opencv/ /usr/local/include/opencv2/
LIBS += -L/usr/local/lib/ \
        -lopencv_core \
        -lopencv_highgui \
        -lopencv_imgproc \
        -lopencv_imgcodecs \

SOURCES += main.cpp\
        mainwindow.cpp \
    image_synthesis.cpp

HEADERS  += mainwindow.h \
    image_synthesis.h

FORMS    += mainwindow.ui
