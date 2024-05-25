#include <iostream>
#include <fstream>
#include <string>
#include "Calibrator.h"
#include "Analyzer.h"


const char * params = "{ help h         |           | Print usage                                    }"
                      "{ @video-file    |           | Path to a video                                }"
                      "{ @dnn-file      |           | File containing the pretrained DNN             }"
                      "{ calib-value    |           | Calibration value                              }"
                      "{ frame-amount   |           | Amount of frames to analyze                    }";


int main(int argc, char* argv[]) {
    cv::CommandLineParser parser(argc, argv, params);
    parser.about("This program analyzes a video of droplets under a microscope.");
    if(parser.has("help"))
    {
        parser.printMessage();
    }

    std::cout << "Mpemba Video Analysis" << std::endl;
    std::cout << "Using OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "." << CV_SUBMINOR_VERSION << std::endl;
    if(!parser.has("@video-file") || !parser.has("@dnn-file"))
    {
        std::cerr << "Parameters missing. Exiting...\n";
        return -1;
    }
    std::string videofile = parser.get<std::string>("@video-file");
    std::string netfile = parser.get<std::string>("@dnn-file");

    double calib_constant = 0;
    if(parser.has("calib-value"))
        calib_constant = parser.get<double>("calib-value");
    else
    {
        cv::VideoCapture video_cap_calib(videofile);
        Calibrator calib(video_cap_calib, 0);
        calib_constant = calib.getCalibrationConstant();
        video_cap_calib.release();
    }

    Analyzer analyzer(videofile, netfile);
    Analyzer::analysisConfig config;
    config.max_movement_threshold_displacement = 10000;
    config.left_border_volume = 0;
    config.right_border_volume = 20000;
    config.calib = calib_constant;
    config.skip_frames_volume = 0;
    config.x_threshold_count = 810;
    config.show_frames_droplets = false;
    config.show_frames_displacement = false;
    config.right_border_displacement = 1800;
    config.confidence_threshold = 0.7;
    analyzer.openCapture();
    analyzer.configure(config);
    int return_code;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    if (parser.has("frame-amount"))
    {
        int num_frames = parser.get<int>("frame-amount");
        return_code = analyzer.analyze(num_frames);
    }
    else
    {
        return_code = analyzer.analyze();
    }
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::cout << "Analysis time:" << std::chrono::duration_cast<std::chrono::seconds>(t2 - t1).count() << "s\n";
    int count = analyzer.getNumDroplets();
    std::cout << "Counted " << count << " droplets" << std::endl;
    std::cout << std::scientific <<  "Calibration constant:" << calib_constant << std::endl;


    return return_code;
}
