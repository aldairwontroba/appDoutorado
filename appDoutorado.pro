QT       += network core gui opengl

LIBS += -LC:\Users\Aldair\GoogleDrive\Doutorado\PROJETOS\qtProjects\appDoutorado -liec61850
LIBS += -LC:\Users\Aldair\GoogleDrive\Doutorado\PROJETOS\qtProjects\appDoutorado -lhal
LIBS += -LC:\Users\Aldair\GoogleDrive\Doutorado\PROJETOS\qtProjects\appDoutorado -lhal-shared
LIBS += -LC:\Users\Aldair\GoogleDrive\Doutorado\PROJETOS\qtProjects\appDoutorado -lwpcap

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
    src/common/inc \
    src/iec61850/inc \
    src/mms/inc \
    src/mms/inc_private \
    src/logging \
    src/sampled_values \
    hal/inc \
    config \
    winpcap/Include \
    winpcap/Include/pcap

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    qcustomplot.cpp \

HEADERS += \
    mainwindow.h \
    qcustomplot.h \

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
