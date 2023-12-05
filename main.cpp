#include <iostream>
#include <fstream>
#include <string>
#include "Calibrator.h"
#include "Analyzer.h"
#include <iterator>
#include <opencv2/dnn.hpp>

const char * params = "{ help h         |           | Print usage }"
                      "{ input          |           | Path to a video  }"
                      "{ calibrate      | no        | Open calibrator for image sequence}"
                      "{ calib-value    | 1.0       | Calibration value (ignored if calibrate is set)}";

int getIntegerAttribute(std::string _filename, std::string _attribute)
{
    std::string line;
    std::ifstream file(_filename);
    if(!file.is_open())
    {
        std::cerr << "Unable to open file" << std::endl;
        return -1;
    }
    while(std::getline(file, line))
    {
        std::stringstream linestream(line);
        std::string segment;
        std::vector<std::string> seglist;
        while (std::getline(linestream, segment, '='))
            seglist.push_back(segment);
        if(seglist[0] == _attribute)
        {
            file.close();
            return std::stoi(seglist[1]);
        }
    }
    return -2;
}

template <typename T>
void writeToFile(std::vector<T> _vec, std::string _filename)
{
    std::ofstream outfile(_filename);
    for (size_t i = 0; i < _vec.size(); ++i) {
            outfile << i << ";" << _vec[i] << "\n";
    }
    outfile.close();
}

const float score_threshold = 0.5;
const float nms_threshold = 0.45;
const float confidence_threshold = 0.45;

void drawLabel(cv::Mat& _input_image, std::string _label, int _left, int _top)
{
    // Display the _label at the _top of the bounding box.
    int baseLine;
    cv::Size label_size = cv::getTextSize(_label, cv::FONT_HERSHEY_SIMPLEX, 0.7, 1, &baseLine);
    _top = std::max(_top, label_size.height);
    // Top _left corner.
    cv::Point tlc = cv::Point(_left, _top);
    // Bottom right corner.
    cv::Point brc = cv::Point(_left + label_size.width, _top + label_size.height + baseLine);
    // Draw white rectangle.
    cv::rectangle(_input_image, tlc, brc, cv::Scalar(0, 0, 0), cv::FILLED);
    // Put the _label on the black rectangle.
    cv::putText(_input_image, _label, cv::Point(_left, _top + label_size.height), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 1);
}

std::vector<cv::Mat> preProcess(cv::Mat & _input_image, cv::dnn::Net & _net, float _size = 640.0)
{
    cv::Mat blob;
    cv::dnn::blobFromImage(_input_image, blob, 1.0/255.0, cv::Size(_size, _size), cv::Scalar(0, 0, 0), true, false);
    _net.setInput(blob);

    std::vector<cv::Mat> outputs;
    _net.forward(outputs, _net.getUnconnectedOutLayersNames());
    return outputs;
}

cv::Mat postProcess(cv::Mat & _input_image, std::vector<cv::Mat> & outputs, const std::vector<std::string> & _class_name, float _size=640)
{
    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    float x_factor = _input_image.cols / _size;
    float y_factor = _input_image.rows / _size;
    float *data = (float *)outputs[0].data;
    const int rows = 25200;
    const int dimensions = 6;

    for (size_t i = 0; i < rows; ++i)
    {
        float confidence = data[4];
        if(confidence >= confidence_threshold)
        {
            float * classes_score = data + 5;
            cv::Mat scores(1, _class_name.size(), CV_32FC1, classes_score);
            cv::Point class_id;
            double max_class_score;
            cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id);
            if(max_class_score > score_threshold)
            {
                confidences.push_back(confidence);
                class_ids.push_back(class_id.x);

                float cx = data[0];
                float cy = data[1];

                float w = data[2];
                float h = data[3];

                int left = int((cx - 0.5 * w) * x_factor);
                int top = int((cy - 0.5 * h) * y_factor);
                int width = int(w * x_factor);
                int height = int(h * y_factor);

                boxes.push_back(cv::Rect(left, top, width, height));
            }
        }
        data += dimensions;
    }
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, score_threshold, nms_threshold, indices);
    for (int i = 0; i < indices.size(); i++)
    {
        int idx = indices[i];
        cv::Rect box = boxes[idx];
        int left = box.x;
        int top = box.y;
        int width = box.width;
        int height = box.height;
        // Draw bounding box.
        cv::rectangle(_input_image, cv::Point(left, top), cv::Point(left + width, top + height), cv::Scalar(255, 0, 0), 3);
        // Get the label for the class name and its confidence.
        std::string label = cv::format("%.2f", confidences[idx]);
        label = _class_name[class_ids[idx]] + ":" + label;
        // Draw class labels.
        drawLabel(_input_image, label, left, top);
    }
    return _input_image;
}

int main(int argc, char* argv[]) {
    cv::CommandLineParser parser(argc, argv, params);
    parser.about("This program analyzes a video of droplets under a microscope.");
    if(parser.has("help"))
    {
        parser.printMessage();
    }

    std::cout << "Mpemba Video Analysis" << std::endl;
    std::cout << "Using OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "." << CV_SUBMINOR_VERSION << std::endl;
    std::string videofile = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/videos/test3_2023-11-26-152108-0000.avi";

    /*Analyzer analyzer(videofile);
    Analyzer::analysisConfig config;
    config.max_movement_threshold_displacement = 100;
    config.left_border_volume = 50;
    config.right_border_volume = 600;
    config.calib = 1.0;
    config.skip_frames_volume = 10;
    config.x_threshold_count = 100;
    config.show_frames_droplets = true;
    config.skip_frames_droplets = 20;
    analyzer.configure(config);
    analyzer.analyze();


    writeToFile(analyzer.getVolumes(),"/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/videos/volumes.csv");
    int count = analyzer.getNumDroplets();
    std::cout << "Counted " << count << " droplets" << std::endl;
*/


    cv::Mat frame = cv::imread("/home/nicholas/Bilder/vlcsnap-2023-11-23-09h51m55s160.png");
    cv::dnn::Net net = cv::dnn::readNet("/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/best.onnx");
    std::vector<cv::Mat> detections;
    detections = preProcess(frame, net);
    std::vector<std::string> classes;
    classes.push_back("droplets");
    cv::Mat result = postProcess(frame, detections, classes);
    cv::imshow("Frame", result);
    cv::waitKey(0);
    return 0;
}
