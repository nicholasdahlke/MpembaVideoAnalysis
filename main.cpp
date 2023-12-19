#include <iostream>
#include <fstream>
#include <string>
#include "Calibrator.h"
#include "Analyzer.h"
#include <iterator>
#include <opencv2/dnn.hpp>

const char * params = "{ help h         |           | Print usage }"
                      "{ input          |           | Path to a video  }"
                      "{ calibrate      | no        | Open calibrator for image sequence}"
                      "{ calib-value    | 1.0       | Calibration value (ignored if calibrate is set)}";

int getIntegerAttribute(std::string _filename, std::string _attribute)
{
    std::string line;
    std::ifstream file(_filename);
    if(!file.is_open())
    {
        std::cerr << "Unable to open file" << std::endl;
        return -1;
    }
    while(std::getline(file, line))
    {
        std::stringstream linestream(line);
        std::string segment;
        std::vector<std::string> seglist;
        while (std::getline(linestream, segment, '='))
            seglist.push_back(segment);
        if(seglist[0] == _attribute)
        {
            file.close();
            return std::stoi(seglist[1]);
        }
    }
    return -2;
}

std::vector<cv::Mat> preProcess(cv::Mat & _input_image, cv::dnn::Net & _net, float _size = 640.0)
{
    cv::Mat blob;
    cv::dnn::blobFromImage(_input_image, blob, 1.0/255.0, cv::Size(_size, _size), cv::Scalar(0, 0, 0), true, false);
    _net.setInput(blob);

    std::vector<cv::Mat> outputs;
    _net.forward(outputs, _net.getUnconnectedOutLayersNames());
    return outputs;
}



int main(int argc, char* argv[]) {
    cv::CommandLineParser parser(argc, argv, params);
    parser.about("This program analyzes a video of droplets under a microscope.");
    if(parser.has("help"))
    {
        parser.printMessage();
    }

    std::cout << "Mpemba Video Analysis" << std::endl;
    std::cout << "Using OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "." << CV_SUBMINOR_VERSION << std::endl;
    std::string videofile = "/home/nicholas/Mpempa Videos/1000Ul Oel 14Ul Wasser 47.81 Fps 2.mp4";
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
