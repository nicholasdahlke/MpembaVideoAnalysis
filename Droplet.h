//
// Created by nicholas on 04.02.24.
//

#ifndef MPEMBAVIDEOANALYSIS_DROPLET_H
#define MPEMBAVIDEOANALYSIS_DROPLET_H
#include <opencv2/core.hpp>

class Droplet {
public:
    Droplet(cv::RotatedRect _ellipse, bool _calculate_volume, bool _is_frozen);
    bool getIsFrozen();
    bool getCalcVolume();
    cv::RotatedRect getEllipse();

private:
    cv::RotatedRect ellipse;
    bool calculate_volume;
    bool is_frozen;
};


#endif //MPEMBAVIDEOANALYSIS_DROPLET_H
