#-------------------------------------------------
#
# Project created by QtCreator 2025-09-09T14:19:31
#
#-------------------------------------------------

QT       += core gui widgets axcontainer printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
CONFIG += c++11 windows
# Windows 编译器上强制按 UTF-8 读源文件（避免中文乱码）
win32: QMAKE_CXXFLAGS += -utf-8
TARGET = IntegratedAlarmSystem
TEMPLATE = app


# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += \
    $$PWD \
    $$PWD/private \
    $$PWD/DeveloperComponents
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        PdfReport.cpp \
        TestSelectionDialog.cpp \
        UserAdminDialog.cpp \
        circlelineedit.cpp \
        flexraythread.cpp \
        main.cpp \
        mainwindow.cpp

HEADERS += \
        PdfReport.h \
        TestSelectionDialog.h \
        UBusCommLibWrapper.h \
        UserAdminDialog.h \
        applogger.h \
        circlelineedit.h \
        flexraythread.h \
        mainwindow.h

FORMS += \
        UserAdminDialog.ui \
        mainwindow.ui

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/./ -lUBusCommLib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/./ -lUBusCommLib
else:unix: LIBS += -L$$PWD/./ -lUBusCommLib

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.


# 头文件路径
INCLUDEPATH += \
    $$PWD/components \
    $$PWD/components/lib \
    $$PWD/adapters

# 1) 编译通用依赖（必须：涟漪、阴影、主题、状态机、动画等）
SOURCES += $$files($$PWD/components/lib/*.cpp, true)
HEADERS += $$files($$PWD/components/lib/*.h,   true)

# 2) 只编译你要的控件
SOURCES += \
    $$PWD/components/qtmaterialflatbutton.cpp \
    $$PWD/components/qtmaterialraisedbutton.cpp \
    $$PWD/components/qtmaterialcheckbox.cpp \
    $$PWD/components/qtmaterialflatbutton_internal.cpp \
        $$PWD/components/qtmaterialtextfield_internal.cpp \
    $$PWD/components/qtmaterialtextfield.cpp

HEADERS += \
    $$PWD/components/qtmaterialflatbutton.h \
    $$PWD/components/qtmaterialraisedbutton.h \
    $$PWD/components/qtmaterialcheckbox.h \
    $$PWD/components/qtmaterialflatbutton_internal.h \
        $$PWD/components/qtmaterialtextfield_internal.h \
    $$PWD/components/qtmaterialtextfield.h

RESOURCES += \
    style.qrc
