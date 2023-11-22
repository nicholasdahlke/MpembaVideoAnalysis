//
// Created by nicholas on 21.11.23.
//

#ifndef MPEMBAVIDEOANALYSIS_ANALYZER_H
#define MPEMBAVIDEOANALYSIS_ANALYZER_H
#include <opencv2/core.hpp>
#include <opencv2/video.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <string>

class Analyzer {
public:
    Analyzer(std::string _filename);

    struct analysisConfig
    {
        bool show_frames_droplets = false;
        bool show_frames_displacement = false;
        float max_movement_threshold_displacement = 1e4;
        bool show_frames_tracking = false;
        int skip_frames_tracking = 0;
        int skip_frames_volume = 0;
        int left_border_volume = 0;
        int right_border_volume = 1e4;
        int x_threshold_count = 0;
        double calib = 1.0;
    };

    int configure(analysisConfig _conf);
    int analyze();
    const std::vector<double> getVolumes();
    int getNumDroplets();

private:
    std::string filename;
    cv::VideoCapture * capture;
    int video_width;
    int video_height;
    int video_frame_count;
    analysisConfig config;
    bool configured = false;

    // Analysis results
    std::vector<std::vector<cv::RotatedRect>> droplet_ellipses;
    std::vector<std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>>> displacement_vectors;
    std::vector<std::vector<cv::Point_<float>>> droplet_tracks; // Untested
    std::vector<double> volumes;
    int num_droplets = 0;


private:
    // Helper functions
    int openCapture();
    void printConfig(Analyzer::analysisConfig _conf);
    void printInfo();
    void clear();
    inline std::string boolToString(bool b);

    // Analysis functions
    int getDropletsFromVideo();
    int getDisplacementVectors();
    int trackDroplet(); // Untested
    int getVolumeFromDroplets();
    double getVolumeFromDroplet(cv::RotatedRect _droplet, double _calib);
    int countDroplets();
};


#endif //MPEMBAVIDEOANALYSIS_ANALYZER_H
