//
// Created by nicholas on 21.11.23.
//

#ifndef MPEMBAVIDEOANALYSIS_ANALYZER_H
#define MPEMBAVIDEOANALYSIS_ANALYZER_H
#include <opencv2/core.hpp>
#include <opencv2/video.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include "Droplet.h"
#include "Detection.h"

class Analyzer {
public:
    Analyzer(const std::filesystem::path& _filename, const std::filesystem::path& _dnn_net_file);
    ~Analyzer();
    struct analysisConfig
    {
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
    };

    int configure(analysisConfig _conf);
    int analyze();
    int analyze(int _num_droplets);
    int getNumDroplets() const;
    int openCapture();


private:

    struct Displacement
    {
        Droplet droplet;
        Droplet droplet_next;
        std::array<double,3> vector;
        int start_frame_number;
    };

    std::filesystem::path filename;
    cv::VideoCapture * capture{};
    double video_width;
    double video_height;
    double video_frame_count;
    analysisConfig config;
    bool configured = false;
    cv::dnn::Net dnn_net;
    cv::Size net_input_size;
    std::vector<std::vector<cv::Mat>> dnn_results;
    std::vector<std::string> dnn_classes;
    std::fstream log_file;
    std::filesystem::path volume_images_path;

    // Analysis results
    std::vector<std::vector<Droplet>> droplet_ellipses;
    std::vector<std::vector<Displacement>> displacement_vectors;
    std::vector<double> volumes;
    std::vector<double> distances;
    std::vector<double> speeds;
    int num_droplets = 0;
    int num_droplets_frozen = 0;


private:
    // Helper functions
    void printConfig(Analyzer::analysisConfig _conf);
    void printInfo();
    void clear();
    static inline std::string boolToString(bool b);
    std::vector<cv::Mat> applyNetToFrame(const cv::Mat& _frame);
    void applyNetToFrames(int _num_droplets);
    static void drawLabel(cv::Mat& _input_image, const std::string& _label, int _left, int _top);
    std::vector<Detection> getBoundingRectFromResults(cv::Mat & _annotation_image, std::vector<cv::Mat> & outputs, const std::vector<std::string> & _class_name, float _size= 640) const;
    void showAllMovementVectors();
    template <typename T>
    void writeToFile(std::vector<T> _vec, std::filesystem::path _filename, std::string _type, std::string _extension);
    template <typename T>
    void writeToFile(T _elem, std::filesystem::path _filename, std::string _type, std::string _extension);
    template <typename T>
    T getMedian(std::vector<T> a);
    cv::Rect enlargeRect(cv::Rect _rect, double _factor);



    // Analysis functions
    int getDropletsFromVideo(int _num_droplets);
    int getDisplacementVectors();
    int getSpeeds();
    int getVolumeFromDroplets();
    static double getVolumeFromDroplet(cv::RotatedRect _droplet, double _calib);
    int countDroplets();
    int measureInterDropletDistances();
};


#endif //MPEMBAVIDEOANALYSIS_ANALYZER_H
