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

class Analyzer {
public:
    Analyzer(const std::filesystem::path& _filename, const std::filesystem::path& _dnn_net_file);
    ~Analyzer();
    struct analysisConfig
    {
        bool show_frames_droplets = true;
        bool show_frames_displacement = true;
        int right_border_displacement = 1e6;
        float max_movement_threshold_displacement = 1e4;
        bool show_frames_tracking = false;
        int skip_frames_tracking = 0;
        int skip_frames_volume = 0;
        int left_border_volume = 0;
        int right_border_volume = 1e4;
        int x_threshold_count = 20;
        double calib = 1.0;
        float score_threshold = 0.5;
        float nms_threshold = 0.45;
        float confidence_threshold = 0.45;
    };

    int configure(analysisConfig _conf);
    int analyze();
    int analyze(int _num_droplets);
    int getNumDroplets() const;
    int openCapture();


private:
    struct Droplet
    {
        cv::RotatedRect ellipse;
        bool calculate_volume;
    };

    struct Displacement
    {
        Droplet droplet;
        Droplet droplet_next;
        std::array<double,3> vector;
        int start_frame_number;
    };

    std::filesystem::path filename;
    cv::VideoCapture * capture{};
    int video_width{};
    int video_height{};
    int video_frame_count{};
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
    std::vector<std::vector<cv::Point_<float>>> droplet_tracks; // Untested
    std::vector<double> volumes;
    std::vector<double> distances;
    int num_droplets = 0;


private:
    // Helper functions
    void printConfig(Analyzer::analysisConfig _conf);
    void printInfo();
    void clear();
    static inline std::string boolToString(bool b);
    std::vector<cv::Mat> applyNetToFrame(const cv::Mat& _frame);
    void applyNetToFrames(int _num_droplets);
    static void drawLabel(cv::Mat& _input_image, const std::string& _label, int _left, int _top);
    std::vector<cv::Rect> getBoundingRectFromResults(cv::Mat & _annotation_image, std::vector<cv::Mat> & outputs, const std::vector<std::string> & _class_name, float _size= 640) const;
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
    int trackDroplet(); // Untested
    int getVolumeFromDroplets();
    static double getVolumeFromDroplet(cv::RotatedRect _droplet, double _calib);
    int countDroplets();
    int measureInterDropletDistances();
    std::vector<std::vector<cv::Point>> filterContours(const std::vector<std::vector<cv::Point>>& _contours, double _max_area);
};


#endif //MPEMBAVIDEOANALYSIS_ANALYZER_H
