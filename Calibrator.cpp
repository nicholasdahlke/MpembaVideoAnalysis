//
// Created by nicholas on 20.11.23.
//

#include "Calibrator.h"
#include <opencv2/core.hpp>
#include <opencv2/video.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

Calibrator::Calibrator(cv::VideoCapture _video, int _frame)
{
    _video.set(cv::CAP_PROP_FRAME_COUNT, _frame);
    _video.read(preview_image_current);
    preview_image_original = preview_image_current.clone();
    std::string window_name = "Calibration Frame";
    cv::namedWindow(window_name);
    cv::setMouseCallback(window_name,dbl_click_callback, this);
    for(;;)
    {
        cv::imshow(window_name, preview_image_current);
        int key = cv::waitKey(20);
        if(key == 255 && crosshairs.size()>0)
        {
            crosshairs.pop_back();
            redrawImage();
        }
        if(key & 0xFF == 27)
            break;
    }
}

void Calibrator::drawCrosshair(cv::Mat &_input, cv::Point _p, cv::Scalar _color)
{
    int length = 10;
    int distance = 3;
    cv::line(_input, cv::Point(_p.x - length - distance, _p.y), cv::Point(_p.x - distance, _p.y), _color, 1);
    cv::line(_input, cv::Point(_p.x + length + distance, _p.y), cv::Point(_p.x + distance, _p.y), _color, 1);
    cv::line(_input, cv::Point(_p.x, _p.y - length - distance), cv::Point(_p.x, _p.y - distance), _color, 1);
    cv::line(_input, cv::Point(_p.x, _p.y + length + distance), cv::Point(_p.x, _p.y + distance), _color, 1);
}

void Calibrator::drawDistanceLine(cv::Mat &_input, cv::Point _p1, cv::Point _p2, cv::Point _p3)
{
    cv::Scalar color(0, 139, 237);
    double a = _p2.y - _p1.y;
    double b = _p1.x - _p2.x;
    double c = _p2.x*_p2.y - _p1.x*_p2.y;
    distance_px = abs(a*_p3.x + b*_p3.y + c)/ sqrt(pow(a, 2)+ pow(b, 2));
    cv::Point text_coord((_p3.x + _p1.x)/2, (_p3.y + _p1.y)/2);
    std::string text = "Dist: " + std::to_string(distance_px) + " px";
    cv::putText(_input, text, text_coord, cv::FONT_HERSHEY_SIMPLEX, 0.5, color);
    char distance_real_cstr[1024];
    FILE *fp = popen("zenity  --title  \"Distance\" --entry --text \"Enter distance in mm here\"", "r");
    fgets(distance_real_cstr, 1025, fp);
    std::string distance_real_str(distance_real_cstr);
    float distance_real = std::stof(distance_real_str);
    std::cout << distance_real_str;
}

void Calibrator::redrawImage()
{
    preview_image_current = preview_image_original.clone();
    for(cv::Point p : crosshairs)
    {
        drawCrosshair(preview_image_current, p, cv::Scalar(0, 0, 255));
    }
    if(crosshairs.size()>=3)
    {
        drawCrosshair(preview_image_current, *(crosshairs.end()-3), cv::Scalar(0, 255, 0));
        drawCrosshair(preview_image_current, *(crosshairs.end()-2), cv::Scalar(0, 255, 0));
        drawCrosshair(preview_image_current, *(crosshairs.end()-1), cv::Scalar(0, 255, 0));

        drawDistanceLine(preview_image_current, *(crosshairs.end()-3),*(crosshairs.end()-2), *(crosshairs.end()-1));
    }
}

void Calibrator::dbl_click_callback(int _event, int _x, int _y, int, void* _calibrator) {
    Calibrator *calibrator = (Calibrator *) _calibrator;
    if (_event == cv::EVENT_LBUTTONUP)
    {
        calibrator->crosshairs.push_back(cv::Point(_x, _y));
        calibrator->redrawImage();
    }
}