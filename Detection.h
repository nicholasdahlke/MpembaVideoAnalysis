//
// Created by nicholas on 04.02.24.
//

#ifndef MPEMBAVIDEOANALYSIS_DETECTION_H
#define MPEMBAVIDEOANALYSIS_DETECTION_H
#include <opencv2/core.hpp>
#include <string>

class Detection {
public:
    Detection(cv::Rect _rect, std::string _detection_type, float _confidence);

    cv::Rect getRect();

    std::string getDetectionType();

    float getConfidence();


private:
    cv::Rect rect;
    std::string detection_type;
    float confidence;
};



#endif //MPEMBAVIDEOANALYSIS_DETECTION_H
