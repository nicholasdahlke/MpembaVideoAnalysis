//
// Created by nicholas on 01.12.24.
//

#include "AnalysisConfig.h"

bool AnalysisConfig::isShowFramesDroplets() const
{
    return show_frames_droplets;
}

void AnalysisConfig::setShowFramesDroplets(bool showFramesDroplets)
{
    show_frames_droplets = showFramesDroplets;
}

bool AnalysisConfig::isShowFramesDisplacement() const
{
    return show_frames_displacement;
}

void AnalysisConfig::setShowFramesDisplacement(bool showFramesDisplacement)
{
    show_frames_displacement = showFramesDisplacement;
}

bool AnalysisConfig::isShowDisplacementVectors() const
{
    return show_displacement_vectors;
}

void AnalysisConfig::setShowDisplacementVectors(bool showDisplacementVectors)
{
    show_displacement_vectors = showDisplacementVectors;
}

int AnalysisConfig::getRightBorderDisplacement() const
{
    return right_border_displacement;
}

void AnalysisConfig::setRightBorderDisplacement(int rightBorderDisplacement)
{
    right_border_displacement = rightBorderDisplacement;
}

float AnalysisConfig::getMaxMovementThresholdDisplacement() const
{
    return max_movement_threshold_displacement;
}

void AnalysisConfig::setMaxMovementThresholdDisplacement(float maxMovementThresholdDisplacement)
{
    max_movement_threshold_displacement = maxMovementThresholdDisplacement;
}

int AnalysisConfig::getSkipFramesVolume() const
{
    return skip_frames_volume;
}

void AnalysisConfig::setSkipFramesVolume(int skipFramesVolume)
{
    skip_frames_volume = skipFramesVolume;
}

int AnalysisConfig::getLeftBorderVolume() const
{
    return left_border_volume;
}

void AnalysisConfig::setLeftBorderVolume(int leftBorderVolume)
{
    left_border_volume = leftBorderVolume;
}

int AnalysisConfig::getRightBorderVolume() const
{
    return right_border_volume;
}

void AnalysisConfig::setRightBorderVolume(int rightBorderVolume)
{
    right_border_volume = rightBorderVolume;
}

int AnalysisConfig::getXThresholdCount() const
{
    return x_threshold_count;
}

void AnalysisConfig::setXThresholdCount(int xThresholdCount)
{
    x_threshold_count = xThresholdCount;
}

double AnalysisConfig::getCalib() const
{
    return calib;
}

void AnalysisConfig::setCalib(double calib)
{
    AnalysisConfig::calib = calib;
}

float AnalysisConfig::getScoreThreshold() const
{
    return score_threshold;
}

void AnalysisConfig::setScoreThreshold(float scoreThreshold)
{
    score_threshold = scoreThreshold;
}

float AnalysisConfig::getNmsThreshold() const
{
    return nms_threshold;
}

void AnalysisConfig::setNmsThreshold(float nmsThreshold)
{
    nms_threshold = nmsThreshold;
}

float AnalysisConfig::getConfidenceThreshold() const
{
    return confidence_threshold;
}

void AnalysisConfig::setConfidenceThreshold(float confidenceThreshold)
{
    confidence_threshold = confidenceThreshold;
}

float AnalysisConfig::getSpeedBorderLeft() const
{
    return speed_border_left;
}

void AnalysisConfig::setSpeedBorderLeft(float speedBorderLeft)
{
    speed_border_left = speedBorderLeft;
}

float AnalysisConfig::getSpeedBorderRight() const
{
    return speed_border_right;
}

void AnalysisConfig::setSpeedBorderRight(float speedBorderRight)
{
    speed_border_right = speedBorderRight;
}

std::filesystem::path AnalysisConfig::getNetPath()
{
    return net_path;
}

void AnalysisConfig::setNetPath(std::filesystem::path netPath)
{
    net_path = netPath;
}

int AnalysisConfig::getFrameBufferSize() const
{
    return frame_buffer_size;
}

void AnalysisConfig::setFrameBufferSize(int frameBufferSize)
{
    frame_buffer_size = frameBufferSize;
}

int AnalysisConfig::getNumFrames() const
{
    return num_frames;
}

void AnalysisConfig::setNumFrames(int numFrames)
{
    num_frames = numFrames;
}
