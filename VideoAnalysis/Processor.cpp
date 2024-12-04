//
// Created by nicholas on 01.12.24.
//

#include "Processor.h"


Processor::Processor(const Measurement &_measurement, const AnalysisConfig &_config)
{
    measurement = _measurement;
    config = _config;
}

void Processor::process()
{
    cv::VideoCapture capture(measurement.getVideoFile());
    if(!capture.isOpened())
        throw std::runtime_error("Error opening video");
    capture.set(cv::CAP_PROP_POS_FRAMES, config.getSkipFramesVolume()-1);
    cv::dnn::Net net = cv::dnn::readNet(config.getNetPath());
    int frame_counter = 0;
    do
    {
        cv::Mat current_frame;
        capture >> current_frame;
        if (current_frame.empty())
            break;
        FrameProcessor fp(current_frame, net, config);
        fp.process();
        Frame frame(frame_counter);
        frame.setDetections(fp.getDetections());
        frame_set.insert(frame);
        frame_counter++;
    }
    while(config.getNumFrames()<0 || frame_set.rbegin()->getFrameNumber() < config.getNumFrames()-1);
}
