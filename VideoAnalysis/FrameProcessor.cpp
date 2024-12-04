//
// Created by nicholas on 01.12.24.
//

#include "FrameProcessor.h"


FrameProcessor::FrameProcessor(const cv::Mat& _frame, const cv::dnn::Net& _net, const AnalysisConfig& _config)
{
    if(_frame.empty())
        throw std::invalid_argument("The passed frame is empty");
    frame = _frame;

    if(_net.empty())
        throw std::invalid_argument("The passed net is empty");
    net = _net;

    config = _config;

    is_processed = false;
}

std::vector<Detection> FrameProcessor::inference()
{
    const cv::Size dnn_size = cv::Size(640.0, 640.0);
    cv::Mat blob = cv::dnn::blobFromImage(frame,
                                          1.0/255.0,
                                          dnn_size,
                                          cv::Scalar(0,0,0),
                                          true,
                                          false);

    std::vector<cv::Mat> net_outputs;
    net.setInput(blob);
    net.forward(net_outputs,
                net.getUnconnectedOutLayersNames());

    // Processing the weird output format of YOLOv5
    const int rows = 25200;
    const int dimensions = 7;
    float y_factor = (float)frame.cols / (float)dnn_size.height;
    float x_factor = (float)frame.rows / (float)dnn_size.width;

    auto* data = (float *)net_outputs[0].data;

    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    for(size_t i = 0; i < rows; i++)
    {
        float confidence = data[4];
        if(confidence >= config.getConfidenceThreshold())
        {
            float* classes_score = data + 5;
            cv::Mat scores(1, (int)dnn_classes.size(), CV_32FC1, classes_score);
            cv::Point class_id;
            double max_class_score;
            cv::minMaxLoc(scores, nullptr, &max_class_score, nullptr, & class_id);
            if(max_class_score > config.getScoreThreshold())
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

                boxes.emplace_back(cv::Point(left, top), cv::Point(left + width, top + height));
            }
        }
        data += dimensions;
    }


    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes,
                      confidences,
                      config.getScoreThreshold(),
                      config.getNmsThreshold(),
                      indices);

    std::vector<Detection> detection_temp;
    for (int idx : indices)
    {
        detection_temp.emplace_back(boxes[idx], dnn_classes[class_ids[idx]], confidences[idx]);
    }
    return detection_temp;
}

std::vector<Droplet> FrameProcessor::getDroplets(const std::vector<Detection>& _detections)
{
    std::vector<Droplet> droplet_temp;
    const float scale_factor = 1.2;
    std::vector<cv::Rect> rect_vector;
    rect_vector.resize(_detections.size());
    std::transform(_detections.begin(), _detections.end(), rect_vector.begin(), [](auto a){return a.getRect();});
    for(Detection detection : _detections)
    {
        cv::Rect detection_larger = enlargeRect(detection.getRect(), scale_factor); //Enlarge rectangle to avoid problematic collisions with borders
        bool skip_droplet = !isInsideFrame(detection_larger) & checkIntersectionWithMargin(detection_larger, rect_vector, scale_factor);
        int x_offset = detection_larger.x;
        int y_offset = detection_larger.y;
        if (!skip_droplet)
        {
            cv::Mat droplet_roi = frame(detection_larger);
            std::vector<cv::Point> droplet_contour = getDropletContourFromROI(droplet_roi);
            if (!droplet_contour.empty())
            {
                // Fit an ellipse
                cv::RotatedRect ellipse_fit = cv::fitEllipse(droplet_contour);

                // Shift the ellipse back to the original position
                ellipse_fit.center.x += x_offset;
                ellipse_fit.center.y += y_offset;

                droplet_temp.emplace_back(ellipse_fit,
                                          true,
                                          detection.getDetectionType() == dnn_classes[1]);
            }
            else
                skip_droplet = true;
        }
        if(skip_droplet)
        {
            int width = detection.getRect().width;
            int height = detection.getRect().height;
            cv::Point center(x_offset + width/2, y_offset + height/2);
            droplet_temp.emplace_back(cv::RotatedRect(center, cv::Size(width, height), 0),
                                      false,
                                      detection.getDetectionType() == dnn_classes[1]);
        }
    }
    return droplet_temp;
}

cv::Rect FrameProcessor::enlargeRect(cv::Rect _rect, double _factor)
{
    if(_factor <= 0)
        throw std::invalid_argument("Invalid scaling factor");
    int pre_width = _rect.width;
    int pre_height = _rect.height;
    _rect.width *= _factor;
    _rect.height *= _factor;
    _rect.x -= (_rect.width-pre_width)/2;
    _rect.y -= (_rect.height-pre_height)/2;
    return _rect;
}

bool FrameProcessor::checkIntersectionWithMargin(const cv::Rect& _rect, const std::vector<cv::Rect>& _rects, float _scale_factor)
{
    // Returns true if the given rect intersects another
    return std::ranges::any_of(_rects, [=](cv::Rect r){return (enlargeRect(r, _scale_factor) & _rect).area() > 0;});
}

bool FrameProcessor::isInsideFrame(const cv::Rect& _rect)
{
    cv::Rect frameRect = cv::Rect(cv::Point(0,0), frame.size());
    return (_rect & frameRect).area() == _rect.area(); //Check if the intersection is the same as the rect
}

std::vector<cv::Point> FrameProcessor::getDropletContourFromROI(cv::Mat _roi)
{
    cv::fastNlMeansDenoising(_roi, _roi, 10, 7, 21);
    cv::Mat grabcut_input;
    float downsampling_factor = 0.5;
    cv::resize(_roi, grabcut_input, cv::Size(0, 0), downsampling_factor, downsampling_factor); // Downscale image for faster grabcut performance

    cv::cvtColor(grabcut_input, grabcut_input, cv::COLOR_GRAY2BGR); // Convert grayscale image to color (necessary for grabcut)
    float scaling_factor = 0.9;
    cv::Mat bgdModel;
    cv::Mat fgdModel;
    cv::Mat outMask;
    cv::Rect grabcut_rect;

    // Define the rectangle containing the droplet
    grabcut_rect.x = (grabcut_input.size[0] * (1 - scaling_factor))/2;
    grabcut_rect.y = (grabcut_input.size[1] * (1 - scaling_factor))/2;
    grabcut_rect.height = grabcut_input.size[0] * scaling_factor;
    grabcut_rect.width = grabcut_input.size[1] * scaling_factor;

    // Apply the grabcut algorithm
    cv::grabCut(grabcut_input,
                outMask,
                grabcut_rect,
                bgdModel,
                fgdModel,
                5,
                cv::GC_INIT_WITH_RECT);

    // Create a lookup table to use the grabcut mask
    cv::Mat lookUpTable(1, 256, CV_8U);
    uint8_t * p = lookUpTable.data;
    for(size_t j = 0; j < 256; ++j)
    {
        p[j] = 255;
    }
    p[0] = 0;
    p[2] = 0;
    cv::LUT(outMask, lookUpTable, outMask);
    // Scale mask to original size
    cv::resize(outMask, outMask, cv::Size(0, 0), 1/downsampling_factor, 1/downsampling_factor);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(outMask, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
    if (contours.empty())
        return {};
    else
    {
        std::vector<cv::Point> contour = *std::max_element(contours.begin(),
                                                           contours.end(),
                                                           [](auto a,
                                                              auto b) -> bool {
                                                               return cv::contourArea(a) <
                                                                      cv::contourArea(b);
                                                           });
        if(contour.size() > 5) // Check if enough points are there
            return contour;
        else
            return {};
    }

}

void FrameProcessor::process()
{
    if(detections.empty())
        processDetections();
    if(droplets.empty())
        processDroplets();
    else
        throw std::runtime_error("You are trying to reprocess an already processed frame");
}

std::vector<Droplet> FrameProcessor::getDroplets()
{
    if (is_processed)
        return droplets;
    else
        throw std::runtime_error("Frame wasn't processed");
}

std::vector<Detection> FrameProcessor::getDetections()
{
    if (is_processed)
        return detections;
    else
        throw std::runtime_error("Frame wasn't processed");
}

void FrameProcessor::processDetections()
{
    detections = inference();
}

void FrameProcessor::processDroplets()
{
    droplets = getDroplets(detections);
}

FrameProcessor::FrameProcessor(const cv::Mat &_frame, const AnalysisConfig &_config,
                               const std::vector<Detection> &_detections)
{
    if(_frame.empty())
        throw std::invalid_argument("The passed frame is empty");
    frame = _frame;

    if(_detections.empty())
        throw std::invalid_argument("The passed detections are empty");
    detections = _detections;

    config = _config;
    is_processed = false;
}

