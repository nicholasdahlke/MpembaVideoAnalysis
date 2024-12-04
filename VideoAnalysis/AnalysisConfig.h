//
// Created by nicholas on 01.12.24.
//

#ifndef MPEMBAVIDEOANALYSIS_ANALYSISCONFIG_H
#define MPEMBAVIDEOANALYSIS_ANALYSISCONFIG_H
#include <filesystem>

class AnalysisConfig
{
public:
    bool isShowFramesDroplets() const;

    void setShowFramesDroplets(bool showFramesDroplets);

    bool isShowFramesDisplacement() const;

    void setShowFramesDisplacement(bool showFramesDisplacement);

    bool isShowDisplacementVectors() const;

    void setShowDisplacementVectors(bool showDisplacementVectors);

    int getRightBorderDisplacement() const;

    void setRightBorderDisplacement(int rightBorderDisplacement);

    float getMaxMovementThresholdDisplacement() const;

    void setMaxMovementThresholdDisplacement(float maxMovementThresholdDisplacement);

    int getSkipFramesVolume() const;

    void setSkipFramesVolume(int skipFramesVolume);

    int getLeftBorderVolume() const;

    void setLeftBorderVolume(int leftBorderVolume);

    int getRightBorderVolume() const;

    void setRightBorderVolume(int rightBorderVolume);

    int getXThresholdCount() const;

    void setXThresholdCount(int xThresholdCount);

    double getCalib() const;

    void setCalib(double calib);

    float getScoreThreshold() const;

    void setScoreThreshold(float scoreThreshold);

    float getNmsThreshold() const;

    void setNmsThreshold(float nmsThreshold);

    float getConfidenceThreshold() const;

    void setConfidenceThreshold(float confidenceThreshold);

    float getSpeedBorderLeft() const;

    void setSpeedBorderLeft(float speedBorderLeft);

    float getSpeedBorderRight() const;

    void setSpeedBorderRight(float speedBorderRight);

    std::filesystem::path getNetPath();

    void setNetPath(std::filesystem::path netPath);

    int getFrameBufferSize() const;

    void setFrameBufferSize(int frameBufferSize);

    int getNumFrames() const;

    void setNumFrames(int numFrames);


private:
    bool show_frames_droplets = false;
    bool show_frames_displacement = false;
    bool show_displacement_vectors = false;
    int right_border_displacement = 1e6;
    float max_movement_threshold_displacement = 1e4;
    int skip_frames_volume = 0;
    int left_border_volume = 5;
    int right_border_volume = 1e4;
    int x_threshold_count = 20;
    double calib = 1.0;
    float score_threshold = 0.5;
    float nms_threshold = 0.4;
    float confidence_threshold = 0.45;
    float speed_border_left = 50;
    float speed_border_right = 50;
    int frame_buffer_size = 50;
    int num_frames = -1;
    std::filesystem::path net_path;

};


#endif //MPEMBAVIDEOANALYSIS_ANALYSISCONFIG_H
