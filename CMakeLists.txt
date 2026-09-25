cmake_minimum_required(VERSION 3.12)

project(ElaFramework VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

set(QT_SDK_DIR "" CACHE PATH "Qt SDK directory")
if(NOT QT_SDK_DIR)
    message(FATAL_ERROR "Set QT_SDK_DIR to the Qt SDK directory")
endif()
option(ELAWIDGETTOOLS_BUILD_EXAMPLE "Build the ElaWidgetTools example" ON)

list(PREPEND CMAKE_PREFIX_PATH "${QT_SDK_DIR}")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets WidgetsPrivate)

if (NOT WIN32)
    add_link_options(-Wl,--disable-new-dtags)
    set(CMAKE_SKIP_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${QT_SDK_DIR}/lib:${CMAKE_INSTALL_PREFIX}/ElaWidgetTools/lib")
endif ()

add_subdirectory(ElaWidgetTools)
if(ELAWIDGETTOOLS_BUILD_EXAMPLE)
    add_subdirectory(ElaWidgetToolsExample)
endif()

if (${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ElaWidgetTools)
endif ()
