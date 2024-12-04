//
// Created by nicholas on 01.12.24.
//

#ifndef MPEMBAVIDEOANALYSIS_MEASUREMENT_H
#define MPEMBAVIDEOANALYSIS_MEASUREMENT_H
#include <filesystem>
#include "Frame.h"
#include <stdexcept>
#include <set>

class Measurement
{
public:
    explicit Measurement(const std::filesystem::path& _video_file);
    Measurement() = default;
    const std::filesystem::path &getVideoFile() const;

private:
    std::filesystem::path video_file;
};


#endif //MPEMBAVIDEOANALYSIS_MEASUREMENT_H
