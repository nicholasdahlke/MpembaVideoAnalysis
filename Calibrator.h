//
// Created by nicholas on 20.11.23.
//

#ifndef MPEMBAVIDEOANALYSIS_CALIBRATOR_H
#define MPEMBAVIDEOANALYSIS_CALIBRATOR_H
#include <opencv2/core.hpp>
#include <opencv2/video.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

class Calibrator {
public:
    Calibrator(cv::VideoCapture _video, int _frame);

private:
    cv::Mat preview_image_current;
    cv::Mat preview_image_original;
    std::vector<cv::Point> crosshairs;
    double distance_px = 0.0;

    void drawCrosshair(cv::Mat & _input, cv::Point _p, cv::Scalar _color);
    void drawDistanceLine(cv::Mat & _input, cv::Point _p1, cv::Point _p2, cv::Point _p3);
    void redrawImage();
    static void dbl_click_callback(int _event, int _x, int _y, int, void* _calibrator);

};


#endif //MPEMBAVIDEOANALYSIS_CALIBRATOR_H
