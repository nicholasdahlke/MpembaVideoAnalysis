//
// Created by nicholas on 01.12.24.
//

#ifndef MPEMBAVIDEOANALYSIS_PROCESSOR_H
#define MPEMBAVIDEOANALYSIS_PROCESSOR_H
#include "Measurement.h"
#include "AnalysisConfig.h"
#include <queue>
#include <set>
#include <opencv2/core.hpp>
#include <opencv2/video.hpp>
#include <opencv2/videoio.hpp>
#include "FrameProcessor.h"

class Processor
{
public:
    Processor(const Measurement &_measurement, const AnalysisConfig &_config);
    void process();

private:
    Measurement measurement;
    AnalysisConfig config;

    std::set<Frame, decltype([](const Frame& a, const Frame& b){
        return a.getFrameNumber() < b.getFrameNumber();
    })> frame_set;

};


#endif //MPEMBAVIDEOANALYSIS_PROCESSOR_H
