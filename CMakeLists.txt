cmake_minimum_required(VERSION 3.5)

project(ddiview VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        ui/uicommon.h
        ui/uicommon.cpp
        ui/mainwindow.cpp
        ui/mainwindow.h
        ui/mainwindow.ui
        ui/statisticsresultdialog.h
        ui/statisticsresultdialog.cpp
        ui/statisticsresultdialog.ui
        ui/articulationtabledialog.h
        ui/articulationtabledialog.cpp
        ui/articulationtabledialog.h
        ui/ddiexportjsonoptionsdialog.h
        ui/ddiexportjsonoptionsdialog.cpp
        ui/ddiexportjsonoptionsdialog.ui

        ui/propertycontextmenu.h
        ui/propertycontextmenu.cpp

        parser/ddi.cpp
        parser/ddi.h

        chunk/propertytype.h
        chunk/propertytype.cpp
        chunk/chunkcreator.h
        chunk/chunkcreator.cpp

        chunk/basechunk.h
        chunk/chunkarray.h
        chunk/dbsinger.h
        chunk/phonemedict.h
        chunk/phonemegroup.h
        chunk/emptychunk.h
        chunk/dbvoice.h
        chunk/dbvarticulation.h
        chunk/dbvarticulationphu.h
        chunk/dbvarticulationphupart.h
        chunk/dbvstationary.h
        chunk/dbvstationaryphu.h
        chunk/dbvstationaryphupart.h
        chunk/dbvvqmorph.h
        chunk/dbvvqmorphphu.h
        chunk/dbvvqmorphphupart.h
        chunk/dbtimbre.h
        chunk/dbtimbremodel.h

        chunk/item_phoneticunit.h
        chunk/item_directory.h
        chunk/item_phonemegroup.h
        chunk/item_groupedphoneme.h
        chunk/item_eprguides.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ddiview
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ddiview APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(ddiview SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(ddiview
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(ddiview PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(ddiview PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ddiview
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ddiview)
endif()
