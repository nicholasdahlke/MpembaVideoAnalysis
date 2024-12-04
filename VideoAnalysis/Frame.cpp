//
// Created by nicholas on 01.12.24.
//

#include "Frame.h"

Frame::Frame(int _frame_number)
{
    frame_number = _frame_number;
    has_detections = false;
    has_droplets = false;
}


int Frame::getFrameNumber() const
{
    return frame_number;
}

const std::vector<Droplet> &Frame::getDroplets() const
{
    return droplets;
}

const std::vector<Detection> &Frame::getDetections() const
{
    return detections;
}

void Frame::setDroplets(std::vector<Droplet> _droplets)
{
    droplets.assign(_droplets.begin(), _droplets.end());
    has_droplets = true;
}

void Frame::setDetections(std::vector<Detection> _detections)
{
    detections.assign(_detections.begin(), _detections.end());
    has_detections = true;
}

bool Frame::hasDroplets()
{
    return has_detections;
}

bool Frame::hasDetections()
{
    return has_detections;
}