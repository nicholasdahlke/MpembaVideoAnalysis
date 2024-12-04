//
// Created by nicholas on 01.12.24.
//

#include "Measurement.h"

Measurement::Measurement(const std::filesystem::path& _video_file)
{
    if (_video_file.empty())
        throw std::invalid_argument("Video file " + _video_file.string() + "does not exist");
    video_file = _video_file;
}

const std::filesystem::path &Measurement::getVideoFile() const
{
    return video_file;
}

