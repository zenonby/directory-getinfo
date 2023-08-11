cmake_minimum_required(VERSION 3.5)

project(GetInfo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_DEBUG_POSTFIX "d")

# N.B. $QT_DIR environment variable must be defined
set (CMAKE_PREFIX_PATH "$ENV{QT_DIR}\\${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}\\msvc2017_64\\")

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

# Build sqlite3 library
add_subdirectory(libs/sqlite3)

set(PROJECT_SOURCES
        main.cpp
        getinfo.cpp
        getinfo.h
        utils.cpp
        utils.h
        settings.cpp
        settings.h
        ProgressDlg.cpp
        ProgressDlg.h
        model/DirectoryStore.cpp
        model/DirectoryStore.h
        model/DirectoryDetails.h
        model/MimeDetails.cpp
        model/MimeDetails.h
        model/WorkStack.cpp
        model/WorkStack.h
        model/DirectoryProcessingStatus.h
        model/DirectoryScanSwitch.cpp
        model/DirectoryScanSwitch.h
        dir_scanner/DirectoryScanner.cpp
        dir_scanner/DirectoryScanner.h
        dir_scanner/IDirectoryScannerEventSink.h
        dir_scanner/KDirectoryInfo.h
        dir_scanner/KMimeSizesInfo.h
        view_model/kfilesystemmodel.cpp
        view_model/kfilesystemmodel.h
        view_model/kmimesizesmodel.cpp
        view_model/kmimesizesmodel.h
        view_model/kmapper.cpp
        view_model/kmapper.h
        getinfo.ui
        resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET GetInfo APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(${PROJECT_NAME}
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets sqlite3)
target_include_directories(${PROJECT_NAME} PRIVATE libs/sqlite3)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
