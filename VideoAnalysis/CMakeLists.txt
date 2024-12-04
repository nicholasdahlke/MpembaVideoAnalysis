cmake_minimum_required(VERSION 3.20)
project(MpembaVideoAnalysis)
set(CMAKE_CXX_STANDARD 23)

find_package(OpenCV REQUIRED)


add_executable(MpembaVideoAnalysis main.cpp Calibrator.cpp Calibrator.h Analyzer.cpp Analyzer.h
        Droplet.cpp
        Droplet.h
        Droplet.cpp
        Droplet.h
        Detection.cpp
        Detection.h
        FrameProcessor.cpp
        FrameProcessor.h
        Measurement.cpp
        Measurement.h
        AnalysisConfig.cpp
        AnalysisConfig.h
        Frame.cpp
        Frame.h
        Processor.cpp
        Processor.h
)
target_link_libraries(MpembaVideoAnalysis ${OpenCV_LIBS})
