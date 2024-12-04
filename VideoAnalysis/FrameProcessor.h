//
// Created by nicholas on 01.12.24.
//

#ifndef MPEMBAVIDEOANALYSIS_FRAMEPROCESSOR_H
#define MPEMBAVIDEOANALYSIS_FRAMEPROCESSOR_H
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#include <stdexcept>
#include "AnalysisConfig.h"
#include <string>
#include "Detection.h"
#include "Droplet.h"
#include <ranges>

class FrameProcessor
{
public:
    FrameProcessor(const cv::Mat& _frame, const cv::dnn::Net& _net, const AnalysisConfig& _config);
    explicit FrameProcessor(const cv::Mat& _frame, const AnalysisConfig& _config, const std::vector<Detection>& _detections);
    void process();
    std::vector<Droplet> getDroplets();
    std::vector<Detection> getDetections();

private:
    cv::Mat frame;
    cv::dnn::Net net;
    AnalysisConfig config;
    std::vector<std::string> dnn_classes = {"droplets", "droplets_frozen"};
    std::vector<Droplet> droplets;
    std::vector<Detection> detections;
    bool is_processed;

    std::vector<Detection> inference();
    std::vector<Droplet> getDroplets(const std::vector<Detection>& _detections);
    static cv::Rect enlargeRect(cv::Rect _rect, double _factor);
    static bool checkIntersectionWithMargin(const cv::Rect& _rect, const std::vector<cv::Rect>& _rects, float _scale_factor);
    bool isInsideFrame(const cv::Rect& _rect);
    static std::vector<cv::Point> getDropletContourFromROI(cv::Mat _roi);
    void processDetections();
    void processDroplets();

};


#endif //MPEMBAVIDEOANALYSIS_FRAMEPROCESSOR_H
