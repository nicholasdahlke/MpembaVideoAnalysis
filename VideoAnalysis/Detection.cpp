//
// Created by nicholas on 04.02.24.
//

#include "Detection.h"

Detection::Detection(cv::Rect _rect, std::string _detection_type, float _confidence)
{
    rect = _rect;
    detection_type = _detection_type;
    confidence = _confidence;
}

cv::Rect Detection::getRect()
{
    return rect;
}

std::string Detection::getDetectionType()
{
    return detection_type;
}

float Detection::getConfidence()
{
    return confidence;
}

bool Detection::operator==(const Detection &a)
{
    return (a.confidence == confidence) && (a.detection_type == detection_type) && (a.rect == rect);
}

bool Detection::operator!=(const Detection &a)
{
    return (a.confidence != confidence) || (a.detection_type != detection_type) || (a.rect != rect);
}
