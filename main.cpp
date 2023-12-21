#include <iostream>
#include <fstream>
#include <string>
#include "Calibrator.h"
#include "Analyzer.h"


const char * params = "{ help h         |           | Print usage                                    }"
                      "{ @video         |           | Path to a video                                }"
                      "{ calibrate      | no        | Open calibrator for image sequence             }"
                      "{ calib-value    | 1.0       | Calibration value (ignored if calibrate is set)}"
                      "{ @dnn-file      |           | File containing the pretrained DNN             }";


int main(int argc, char* argv[]) {
    cv::CommandLineParser parser(argc, argv, params);
    parser.about("This program analyzes a video of droplets under a microscope.");
    if(parser.has("help"))
    {
        parser.printMessage();
    }

    std::cout << "Mpemba Video Analysis" << std::endl;
    std::cout << "Using OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "." << CV_SUBMINOR_VERSION << std::endl;
    std::string videofile = "/home/nicholas/Mpempa Videos/400Ul Oel 14Ul Wasser 47.81Fps.mp4";
    std::string netfile = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/best_new.onnx";
    cv::VideoCapture video_cap_calib(videofile);
    Calibrator calib(video_cap_calib, 0);
    double calib_constant = calib.getCalibrationConstant();
    video_cap_calib.release();

    Analyzer analyzer(videofile, netfile);
    Analyzer::analysisConfig config;
    config.max_movement_threshold_displacement = 10000;
    config.left_border_volume = 0;
    config.right_border_volume = 20000;
    config.calib = calib_constant;
    config.skip_frames_volume = 0;
    config.x_threshold_count = 810;
    config.show_frames_droplets = true;
    config.show_frames_displacement = false;
    config.right_border_displacement = 1800;
    config.confidence_threshold = 0.7;
    analyzer.openCapture();
    analyzer.configure(config);
    analyzer.analyze(50);

    int count = analyzer.getNumDroplets();
    std::cout << "Counted " << count << " droplets" << std::endl;
    return 0;
}
