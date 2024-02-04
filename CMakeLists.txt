cmake_minimum_required(VERSION 3.25)
project(MpembaVideoAnalysis)
set(CMAKE_CXX_STANDARD 23)

find_package(OpenCV REQUIRED)
find_package(TBB REQUIRED)


add_executable(MpembaVideoAnalysis main.cpp Calibrator.cpp Calibrator.h Analyzer.cpp Analyzer.h
        Droplet.cpp
        Droplet.h
        Droplet.cpp
        Droplet.h
        Detection.cpp
        Detection.h)
target_link_libraries(MpembaVideoAnalysis ${OpenCV_LIBS} TBB::tbb bgslibrary_core)
