QT       += core gui
QT       += network xml charts
QT       += serialport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    code.cpp \
    ftpserver.cpp \
    main.cpp \
    mainwindow.cpp \
    prodatasend.cpp \
    pvariable.cpp \
    qextserialbase.cpp \
    systemset.cpp \
    udpthread.cpp \
    win_qextserialport.cpp

HEADERS += \
    code.h \
    ftpserver.h \
    mainwindow.h \
    prodatasend.h \
    pvariable.h \
    qextserialbase.h \
    systemset.h \
    udpthread.h \
    win_qextserialport.h

FORMS += \
    code.ui \
    mainwindow.ui \
    systemset.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    pic.qrc

DISTFILES += \
    icon/1-start.png \
    icon/10-程引.png \
    icon/10-程引（press）.png \
    icon/10-程引（禁止）.png \
    icon/11-源码.png \
    icon/11-源码（press）.png \
    icon/11-源码（禁止）.png \
    icon/13-设置icon.png \
    icon/2-Remote.png \
    icon/3-Home.png \
    icon/3-Home（禁止）.png \
    icon/4_Stow.png \
    icon/4_Stow（press）.png \
    icon/4_Stow（禁止）.png \
    icon/5-Record.png \
    icon/5-Record（press）.png \
    icon/5-Record（禁止）.png \
    icon/6-Read.png \
    icon/6-Read（press）.png \
    icon/6-Read（禁止）.png \
    icon/7-Exit（press）.png \
    icon/7-Exit（禁止）.png \
    icon/7_Exit.png \
    icon/8-按钮背景图.png \
    icon/8-校标.png \
    icon/8-校标（press）.png \
    icon/8-校标（禁止）.png \
    icon/9-ACU.png \
    icon/9-ACU（press）.png \
    icon/9-ACU（禁止）.png \
    icon/Home-press.png \
    icon/Remote-dis.png \
    icon/Remote-press.png \
    icon/buttom.png \
    icon/green.png \
    icon/grey.png \
    icon/header.png \
    icon/red.png \
    icon/start-dis.png \
    icon/start-press.png \
    icon/start.png \
    icon/tubiao.png \
    icon/yellow.png \
    icon/向上.png \
    icon/向上_press.png \
    icon/向下.png \
    icon/向下_press.png \
    icon/向右-2.png \
    icon/向右-2_press.png \
    icon/向右.png \
    icon/向左-2.png \
    icon/向左-2_press.png \
    icon/向左.png \
    icon/重启.png \
    icon/重启_press.png
