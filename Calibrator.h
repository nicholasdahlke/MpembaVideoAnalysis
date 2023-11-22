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
    double getCalibrationConstant();

private:

    cv::Mat preview_image_current;
    cv::Mat preview_image_original;
    double distance_px = 0.0;
    double distance_mm = 0.0;
    cv::RotatedRect tubeRect;
    int rect_angle = 0;
    int rect_rotation = 0;
    int rect_height;
    int rect_width;
    int rect_translation_y;

    int frame_width;
    int frame_height;

    void redrawImage();
    static void rotation_tr_callback(int value, void* data);
    static void translation_y_tr_callback(int value, void* data);
    static void height_tr_callback(int value, void* data);

    static cv::RotatedRect makeRect(Calibrator * calibrator);
    void showDistancePopup();
};


#endif //MPEMBAVIDEOANALYSIS_CALIBRATOR_H
