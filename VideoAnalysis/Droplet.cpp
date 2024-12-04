//
// Created by nicholas on 04.02.24.
//

#include "Droplet.h"

Droplet::Droplet(cv::RotatedRect _ellipse, bool _calculate_volume, bool _is_frozen)
{
    ellipse = _ellipse;
    calculate_volume = _calculate_volume;
    is_frozen = _is_frozen;
}

bool Droplet::getIsFrozen()
{
    return is_frozen;
}

bool Droplet::getCalcVolume()
{
    return calculate_volume;
}

cv::RotatedRect Droplet::getEllipse()
{
    return ellipse;
}

