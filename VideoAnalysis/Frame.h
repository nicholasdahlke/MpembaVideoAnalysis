//
// Created by nicholas on 01.12.24.
//

#ifndef MPEMBAVIDEOANALYSIS_FRAME_H
#define MPEMBAVIDEOANALYSIS_FRAME_H
#include "Droplet.h"
#include "Detection.h"
#include <vector>

class Frame
{
public:
    explicit Frame(int _frame_number);

    int getFrameNumber() const;
    const std::vector<Droplet> &getDroplets() const;
    const std::vector<Detection> &getDetections() const;
    void setDroplets(std::vector<Droplet> _droplets);
    void setDetections(std::vector<Detection> _detections);
    bool hasDroplets();
    bool hasDetections();


private:
    int frame_number;
    std::vector<Droplet> droplets;
    std::vector<Detection> detections;
    bool has_droplets{};
    bool has_detections{};
};


#endif //MPEMBAVIDEOANALYSIS_FRAME_H
