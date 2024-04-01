QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += core widgets charts

CONFIG += c++20
QMAKE_CXXFLAGS += -std=c++2a

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    getinfo.cpp \
    utils.cpp \
    settings.cpp \
    ProgressDlg.cpp \
    FileSizeDivisor.cpp \
    KDateTimeSeriesChartView.cpp \
    model/DirectoryStore.cpp \
    model/MimeDetails.cpp \
    model/WorkStack.cpp \
    model/DirectoryScanSwitch.cpp \
    model/HistoryProvider.cpp \
    view_model/kfilesystemmodel.cpp \
    view_model/kmimesizesmodel.cpp \
    view_model/kmapper.cpp \
    view_model/kdatetimeserieschartmodel.cpp \
    dir_scanner/DirectoriesScanOrchestrator.cpp \
    dir_scanner/DirectoryScanner.cpp

HEADERS += \
    getinfo.h \
    utils.h \
    settings.h \
    ProgressDlg.h \
    FileSizeDivisor.h \
    KDateTimeSeriesChartView.h \
    model/DirectoryDetails.h \
    model/DirectoryProcessingStatus.h \
    model/DirectoryStore.h \
    model/MimeDetails.h \
    model/WorkStack.h \
    model/DirectoryScanSwitch.h \
    model/HistoryProvider.h \
    view_model/kfilesystemmodel.h \
    view_model/kmimesizesmodel.h \
    view_model/kmapper.h \
    view_model/kdatetimeserieschartmodel.h \
    dir_scanner/DirectoriesScanOrchestrator.h \
    dir_scanner/DirectoryScanner.h \
    dir_scanner/IDirectoryScannerEventSink.h \
    dir_scanner/KDirectoryInfo.h \
    dir_scanner/KMimeSizesInfo.h

LIBS += \
    -L$$PWD/libs/yasw -lyasw

FORMS += \
    getinfo.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
