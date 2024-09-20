cmake_minimum_required(VERSION 3.20)
project(MpembaGUI)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_executable(MpembaGUI main.cpp
        Window.cpp
        Window.h
        SetupPropertiesWidget.cpp
        SetupPropertiesWidget.h
        RunPropertiesWidget.cpp
        RunPropertiesWidget.h
        FilePropertiesWidget.cpp
        FilePropertiesWidget.h
        Measurement.cpp
        Measurement.h
)
target_link_libraries(MpembaGUI Qt5::Widgets)
