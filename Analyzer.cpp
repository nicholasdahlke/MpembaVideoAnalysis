//
// Created by nicholas on 21.11.23.
//

#include "Analyzer.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <opencv2/bgsegm.hpp>
#include <cmath>
#include <opencv2/intensity_transform.hpp>

Analyzer::Analyzer(const std::filesystem::path& _filename, const std::filesystem::path& _net_filename)
{
    log_file.open(_filename.parent_path().string() + "/" + _filename.stem().string() + ".log", std::fstream::out | std::fstream::app);
    if (_filename.empty() || _net_filename.empty())
    {
        std::cerr << "Error:empty filename" << std::endl;
        log_file << "Error:empty filename" << "\n";
    }
    else
    {
        filename = _filename;
        dnn_net = cv::dnn::readNet(_net_filename);
    }
    net_input_size = cv::Size(640.0, 640.0);
    dnn_classes.emplace_back("droplets");
}


int Analyzer::openCapture()
{
    if(filename.empty())
    {
        std::cerr << "Error: Filename empty" << std::endl;
        log_file << "Error: Filename empty" << "\n";
        return -1;
    }

    capture = new cv::VideoCapture(filename);
    if(!capture->isOpened())
    {
        std::cerr << "Error: Unable to open " << filename << std::endl;
        log_file << "Error: Unable to open " << filename << "\n";
        return -2;
    }

    video_height = capture->get(cv::CAP_PROP_FRAME_HEIGHT);
    video_width = capture->get(cv::CAP_PROP_FRAME_WIDTH);
    video_frame_count = capture->get(cv::CAP_PROP_FRAME_COUNT);
    if(video_frame_count <= 0)
    {
        std::cerr << "Error: No frames found (Frame count is 0)" << std::endl;
        log_file << "Error: No frames found (Frame count is 0)" << "\n";
        return -1;
    }

    std::cout << "Video with dimensions (w x h):" << video_width << "x" << video_height << std::endl;
    log_file << "Video with dimensions (w x h):" << video_width << "x" << video_height << "\n";

    return 0;
}

int Analyzer::getDropletsFromVideo(int _num_droplets)
{
    capture->set(cv::CAP_PROP_POS_FRAMES, 0);
    for (size_t i = 0; i < _num_droplets; i++)
    {
        cv::Mat current_frame;
        *capture >> current_frame;
        if(current_frame.empty())
            break;
        std::vector<cv::Mat> current_detections = dnn_results[i];
        cv::Mat current_annotated = current_frame.clone();
        std::vector<cv::Rect> detections = getBoundingRectFromResults(current_annotated, current_detections, dnn_classes);
        cv::cvtColor(current_frame, current_frame, cv::COLOR_BGR2GRAY);
        std::vector<Droplet> ellipses;
        for (cv::Rect detection : detections)
        {
            detection = enlargeRect(detection, 1.2); //Enlarge rectangle to avoid problematic collisions with borders
            cv::rectangle(current_annotated, detection, cv::Scalar(0, 255, 0), 2);
            bool skip_droplet = false;
            if(detection.x < 0)
                skip_droplet = true;
            if(detection.y < 0)
                skip_droplet = true;
            if(detection.x + detection.width > video_width)
                skip_droplet = true;
            if(detection.y + detection.height > video_height)
                skip_droplet = true;
            int x_offset = detection.x;
            int y_offset = detection.y;
            std::cout << "ROI x" << std::to_string(detection.x) << "\n";
            std::cout << "ROI y" << std::to_string(detection.y) << "\n";
            std::vector<std::vector<cv::Point>> contours;
            if(!skip_droplet)
            {
                cv::Mat droplet_roi = current_frame(detection);
                cv::adaptiveThreshold(droplet_roi, droplet_roi, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 21, 2);
                cv::morphologyEx(droplet_roi, droplet_roi, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5)), cv::Point(-1, -1), 1);
                cv::findContours(droplet_roi, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0,0));
                double max_drop_area = std::numbers::pi * detection.height * detection.width * 1.2;
                contours = filterContours(contours, max_drop_area);
                // Shift contours
                for(std::vector<cv::Point>& contour: contours)
                {
                    for(cv::Point& point : contour)
                    {
                        point.x += x_offset;
                        point.y += y_offset;
                    }
                }
                cv::drawContours(current_annotated, contours, -1, cv::Scalar(255, 125, 0));
            }
            if(contours.size() == 2 && !skip_droplet)
            {//This case is executed if it is possible to fit an ellipse
                std::vector<cv::Point> max_contour = contours.front();
                if (cv::contourArea(contours.front()) < cv::contourArea(contours.back()))
                    max_contour = contours.back();
                std::vector<std::vector<cv::Point>> cont_vec;
                cv::RotatedRect drop_ellipse = cv::fitEllipse(max_contour);
                //drop_ellipse.center.x += x_offset;
                //drop_ellipse.center.y += y_offset;
                ellipses.push_back((Droplet){drop_ellipse, true});
            }
            else
            {
                int width = detection.width;
                int height = detection.height;
                cv::Point center(x_offset + width/2, y_offset + height/2);
                ellipses.push_back((Droplet){cv::RotatedRect(center, cv::Size(width, height), 0), false});

            }
        }
        if(config.show_frames_droplets)
        {
            for(Droplet droplet : ellipses)
            {
                cv::Scalar color(0, 0, 255);
                if(droplet.calculate_volume)
                    color = cv::Scalar(0, 255, 0);
                cv::ellipse(current_annotated, droplet.ellipse, color);
            }
            std::cout << "Frame end\n\n";
            cv::imshow("Frames", current_annotated);
            cv::waitKey(500);
        }
        droplet_ellipses.push_back(ellipses);
    }
    return 0;
}

std::vector<std::vector<cv::Point>> Analyzer::filterContours(const std::vector<std::vector<cv::Point>>& _contours, double _max_area)
{
    std::vector<std::vector<cv::Point>> filtered_contours;
    std::vector<double> contour_areas;
    for (const std::vector<cv::Point>& contour : _contours)
    {
        if(contour.size() != 4)
        {
            filtered_contours.push_back(contour);
            contour_areas.push_back(cv::contourArea(contour));
        }
    }
    double contour_median_lower = 0.1 * getMedian(contour_areas);
    std::vector<std::vector<cv::Point>>::iterator contour_it;
    for(contour_it = filtered_contours.begin(); contour_it != filtered_contours.end();)
    {
        double contour_area = cv::contourArea(*contour_it);
        if(contour_area < contour_median_lower || contour_area > _max_area)
            contour_it = filtered_contours.erase(contour_it);
        else
            ++contour_it;
    }
    return filtered_contours;
}

int Analyzer::getDisplacementVectors()
{
    for (size_t i = 0; i < droplet_ellipses.size() - 1; i++)
    {
        std::cout << "Calculating displacement vectors for frame " << i << "\n";
        log_file << "Calculating displacement vectors for frame " << i << "\n";

        cv::Mat preview_image = cv::Mat::zeros(cv::Size(video_width, video_height), CV_8UC3);
        std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>> displacement_center;

        for(Droplet droplet : droplet_ellipses[i])
        {
            cv::ellipse(preview_image, droplet.ellipse, cv::Scalar (0, 255, 0), 2);
            cv::circle(preview_image, droplet.ellipse.center, 5, (0, 0, 255), -1);

            cv::Point curr_point = droplet.ellipse.center;
            if(curr_point.x > config.right_border_displacement)
            {
                std::cerr << "Droplet behind border, displacement calculation will most likely fail:SKIPPING" << std::endl;
                log_file << "Droplet behind border, displacement calculation will most likely fail:SKIPPING" << "\n";
            }
            else
            {
                std::vector<std::array<double, 3>> displacementVectors; // x-vector component, y-vector component, distance
                for(Droplet ellipse_next : droplet_ellipses[i + 1])
                {
                    std::array<double, 3> displacement{0, 0};
                    if((ellipse_next.ellipse.center.x - curr_point.x) > 0 && (ellipse_next.ellipse.center.x - curr_point.x) < config.max_movement_threshold_displacement) { // Allow only positive displacement lower than the maximum allowed movement
                        displacement[0] = ellipse_next.ellipse.center.x - curr_point.x;
                        displacement[1] = ellipse_next.ellipse.center.y - curr_point.y;
                        displacement[2] = sqrt(pow(displacement[0], 2) + pow(displacement[1], 2));
                        displacementVectors.push_back(displacement);
                    }
                }

                if(!displacementVectors.empty())
                {
                    std::sort(displacementVectors.begin(), displacementVectors.end(), [&](std::array<double, 3> & a, std::array<double, 3> & b)->bool{return a[2]>b[2];});
                    cv::Point displacedPoint;
                    displacedPoint.x = curr_point.x + displacementVectors.back()[0];
                    displacedPoint.y = curr_point.y + displacementVectors.back()[1];

                    cv::line(preview_image, curr_point, displacedPoint, cv::Scalar(0, 0, 255), 1);
                    displacement_center.emplace_back(droplet.ellipse, displacementVectors.back());
                }
                else
                {
                    std::cerr << "Calculation of displacement vectors not possible for frame " << std::to_string(i) << std::endl;
                    log_file << "Calculation of displacement vectors not possible for frame " << std::to_string(i) << "\n";

                }
            }

        }

        if(config.show_frames_displacement)
        {
            for(Droplet droplet : droplet_ellipses[i + 1])
            {
                cv::ellipse(preview_image, droplet.ellipse, cv::Scalar (255, 0, 0), 2);
                cv::circle(preview_image, droplet.ellipse.center, 5, (0, 0, 255), -1);
            }
            cv::imshow("Frame", preview_image);
            int keyboard = cv::waitKey(100);
            if (keyboard == 'q' || keyboard == 27)
                break;
        }
        displacement_vectors.push_back(displacement_center);
    }
    return 0;
}

int Analyzer::trackDroplet()
{

    auto iter = displacement_vectors.begin();
    std::vector<cv::Point_<float>> visited;
    std::advance(iter, config.skip_frames_tracking);
    int frame_number = 0;
    while (iter != displacement_vectors.end())
    {
        std::cout << "Tracking for frame " << frame_number << "\n";
        log_file << "Tracking for frame " << frame_number << "\n";

        frame_number++;
        for(std::tuple<cv::RotatedRect, std::array<double, 3>> displacement : *iter)
        {
            if(std::find(visited.begin(), visited.end(), std::get<0>(displacement).center) == visited.end())
            {
                std::vector<cv::Point_<float>> droplet_track;
                droplet_track.push_back(std::get<0>(displacement).center);
                auto next_iter =
                        iter + 1;
                while (!next_iter->empty() && next_iter != displacement_vectors.end()) {
                    for (std::tuple<cv::RotatedRect, std::array<double, 3>> displacement_next: *next_iter) {
                        if (std::find(visited.begin(), visited.end(), std::get<0>(displacement_next).center) == visited.end())
                        {
                            int dx = droplet_track.back().x - std::get<0>(displacement_next).center.x;
                            int dy = droplet_track.back().y - std::get<0>(displacement_next).center.y;
                            if (dx == 0 && dy == 0) {
                                droplet_track.push_back(std::get<0>(displacement_next).center);
                                break;
                            }
                        }
                    }
                    std::advance(next_iter, 1);
                }
                droplet_tracks.push_back(droplet_track);
                visited.insert(visited.end(), droplet_track.begin(), droplet_track.end());
            }

        }
        std::advance(iter, 1);
    }
    return 0;
}

int Analyzer::getVolumeFromDroplets()
{
    auto frame_iter = droplet_ellipses.begin();
    std::advance(frame_iter, config.skip_frames_volume);
    int framenumber = config.skip_frames_volume;
    while(frame_iter != droplet_ellipses.end())
    {
        std::cout << "Analyzing frame " << framenumber << " with " << frame_iter->size() << " droplets\n";
        log_file << "Analyzing frame " << framenumber << " with " << frame_iter->size() << " droplets\n";

        for(Droplet droplet : *frame_iter)
        {
            if (droplet.ellipse.center.x > config.left_border_volume && droplet.ellipse.center.x < config.right_border_volume && droplet.calculate_volume)
            {
                volumes.push_back(getVolumeFromDroplet(droplet.ellipse, config.calib));
            }
        }
        ++frame_iter;
        framenumber++;
    }
    return 0;
}

double Analyzer::getVolumeFromDroplet(cv::RotatedRect _droplet, double _calib)
{
    // The shape of the droplet is approximated as an elongated rotational ellipsoid V=(4/3)*pi*a^2*c#

    float c = std::max(_droplet.size.width, _droplet.size.height) / 2.0;
    float a = std::min(_droplet.size.width, _droplet.size.height) / 2.0;
    a = a*_calib;
    c = c*_calib;
    return (4.0/3.0) * std::numbers::pi * std::pow(a, 2) * c;
}

int Analyzer::countDroplets()
{
    std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>> total_displacements;
    for(const std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>>& a : displacement_vectors)
    {
        for(std::tuple<cv::RotatedRect, std::array<double, 3>> b : a)
        {
            total_displacements.push_back(b);
        }
    }
    int count = 0;
    for(std::tuple<cv::RotatedRect, std::array<double, 3>> displacement : total_displacements)
    {
        if(std::get<0>(displacement).center.x < config.x_threshold_count && (std::get<0>(displacement).center.x + std::get<1>(displacement)[0]) > config.x_threshold_count)
            count++;
    }
    num_droplets = count;
    return 0;
}

int Analyzer::configure(Analyzer::analysisConfig _conf)
{
    config = _conf;
    configured = true;
    printInfo();
    return 0;
}

void Analyzer::printConfig(Analyzer::analysisConfig _conf)
{
    std::string config_string = "show_frames_droplets=" + boolToString(_conf.show_frames_droplets) + "\n"
                              + "show_frames_displacement=" + boolToString(_conf.show_frames_displacement) + "\n"
                              + "right_border_displcement=" + std::to_string(_conf.right_border_displacement) + "\n"
                              + "max_movement_threshold_displacement=" + std::to_string(_conf.max_movement_threshold_displacement) + "\n"
                              + "show_frames_tracking=" + boolToString(_conf.show_frames_tracking) + "\n"
                              + "skip_frames_tracking=" + std::to_string(_conf.skip_frames_tracking) + "\n"
                              + "skip_frames_volume=" + std::to_string(_conf.skip_frames_volume) + "\n"
                              + "left_border_volume=" + std::to_string(_conf.left_border_volume) + "\n"
                              + "right_border_volume=" + std::to_string(_conf.right_border_volume) + "\n"
                              + "x_threshold_count=" + std::to_string(_conf.x_threshold_count) + "\n"
                              + "calib=" + std::to_string(_conf.calib) + "\n"
                              + "score_threshold=" + std::to_string(_conf.score_threshold) + "\n"
                              + "nms_threshold=" + std::to_string(_conf.nms_threshold) + "\n"
                              + "confidence_threshold" + std::to_string(_conf.confidence_threshold) + "\n";

    std::cout << config_string;
    log_file << config_string;
}

void Analyzer::printInfo()
{
    std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %X");
    std::string info_string = "time=" + ss.str() + "\n"
                              + "file=" + filename.string() + "\n"
                              + "width=" + std::to_string(video_width) + "\n"
                              + "height=" + std::to_string(video_height) + "\n"
                              + "frames=" + std::to_string(video_frame_count) + "\n";
    std::cout << info_string;
    log_file << info_string;
    printConfig(config);
}

inline std::string Analyzer::boolToString(bool b)
{
    return b ? "Yes" : "No";
}

int Analyzer::analyze()
{
    return analyze(video_frame_count);
}

int Analyzer::analyze(int _num_droplets)
{
    if (configured)
    {
        int error_code = 0;

        if(error_code != 0)
            return -1;

        applyNetToFrames(_num_droplets);

        error_code = getDropletsFromVideo(_num_droplets);
        if (error_code != 0)
            return -1;

        error_code = getDisplacementVectors();
        if (error_code != 0)
            return -1;

        error_code = getVolumeFromDroplets();
        if(error_code != 0)
            return -1;

        error_code = countDroplets();
        if(error_code != 0)
            return -1;

        std::cout << "Analysis finished, writing results to file" << std::endl;
        log_file << "Analysis finished, writing results to file" << "\n";
        writeToFile(volumes, filename, "volumes", ".csv");
        writeToFile(num_droplets, filename, "droplet_count", ".csv");
    }
    else
    {
        std::cerr << "Error: not configured" << std::endl;
        log_file << "Error: not configured" << "\n";
        return -2;
    }
    return 0;
}

void Analyzer::clear()
{
    droplet_ellipses.clear();
    displacement_vectors.clear();
    droplet_tracks.clear();
    volumes.clear();
    num_droplets = 0;
}

int Analyzer::getNumDroplets() const
{
    return num_droplets;
}

std::vector<cv::Mat> Analyzer::applyNetToFrame(const cv::Mat& _frame)
{
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    std::cout << "-----------------------\n" << "Generating blob\n";
    log_file << "-----------------------\n" << "Generating blob\n";

    cv::Mat blob;
    blob = cv::dnn::blobFromImage(_frame, 1.0/255.0, net_input_size, cv::Scalar(0,0,0), true, false);
    std::vector<cv::Mat> outputs;
    dnn_net.setInput(blob);
    dnn_net.forward(outputs, dnn_net.getUnconnectedOutLayersNames());
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::cout << "DNN Inference time:" << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() / (double)video_frame_count << "ms\n" << "-----------------------\n";
    log_file << "DNN Inference time:" << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() / (double)video_frame_count << "ms\n" << "-----------------------\n";

    return outputs;
}



void Analyzer::applyNetToFrames(int _num_droplets)
{
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    int frame_number = 0;
    for(size_t i = 0; i < _num_droplets; i++)
    {
        cv::Mat current_frame;
        *capture >> current_frame;
        if(current_frame.empty())
            break;
        dnn_results.push_back(applyNetToFrame(current_frame));
        float progress = ((float)frame_number / (float)_num_droplets)*100.0;
        frame_number++;
        std::cout << "Read frame number " << std::to_string(frame_number) << " (progress is " << std::to_string(progress) << "%)\n";
        log_file << "Read frame number " << std::to_string(frame_number) << " (progress is " << std::to_string(progress) << "%)\n";

    }
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::cout << "Total DNN Inference time:" << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "ms" << std::endl;
    log_file << "Total DNN Inference time:" << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << "ms" << std::endl;

}

void Analyzer::drawLabel(cv::Mat& _input_image, const std::string& _label, int _left, int _top)
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

std::vector<cv::Rect> Analyzer::getBoundingRectFromResults(cv::Mat & _annotation_image, std::vector<cv::Mat> & outputs, const std::vector<std::string> & _class_name, float _size) const
{
    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    float x_factor = _annotation_image.cols / _size;
    float y_factor = _annotation_image.rows / _size;
    float *data = (float *)outputs[0].data;
    const int rows = 25200;
    const int dimensions = 6;

    for (size_t i = 0; i < rows; ++i)
    {
        float confidence = data[4];
        if(confidence >= config.confidence_threshold)
        {
            float * classes_score = data + 5;
            cv::Mat scores(1, _class_name.size(), CV_32FC1, classes_score);
            cv::Point class_id;
            double max_class_score;
            cv::minMaxLoc(scores, 0, &max_class_score, 0, &class_id);
            if(max_class_score > config.score_threshold)
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

                boxes.push_back(cv::Rect(cv::Point(left, top), cv::Point(left + width, top + height)));
            }
        }
        data += dimensions;
    }
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, config.score_threshold, config.nms_threshold, indices);
    std::vector<cv::Rect> bounding_rects;
    for (int i = 0; i < indices.size(); i++)
    {
        int idx = indices[i];
        cv::Rect box = boxes[idx];
        int left = box.x;
        int top = box.y;

        //cv::Rect current_bounding_rect = box;
        cv::rectangle(_annotation_image, box, cv::Scalar(255, 0, 0), 3);
        bounding_rects.push_back(box);
        // Get the label for the class name and its confidence.
        std::string label = cv::format("%.2f", confidences[idx]);
        label = _class_name[class_ids[idx]] + ":" + label;
        // Draw class labels.
        drawLabel(_annotation_image, label, left, top);
    }
    return bounding_rects;
}
void Analyzer::showAllMovementVectors()
{
    cv::RNG rng;
    cv::Mat preview_image = cv::Mat::zeros(cv::Size(video_width, video_height), CV_8UC3);
    for(std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>> image_displ : displacement_vectors)
    {
        for (std::tuple<cv::RotatedRect, std::array<double, 3>> displ_vec : image_displ)
        {
            cv::Point curr_point = std::get<0>(displ_vec).center;
            std::array<double, 3> displ_vec_array = std::get<1>(displ_vec);
            cv::Point displaced_point;
            displaced_point.x = curr_point.x + displ_vec_array[0];
            int rand_shift = rng.uniform(-200,  200);
            displaced_point.y = curr_point.y + displ_vec_array[1] + rand_shift;
            curr_point.y = curr_point.y + rand_shift;
            cv::line(preview_image, curr_point, displaced_point, cv::Scalar(rng.uniform(0,255), rng.uniform(0,rng.uniform(0,255)), 255), 1);
        }
    }
    cv::imshow("Disp Vectors", preview_image);
    cv::waitKey();
}

Analyzer::~Analyzer()
{
    log_file.close();
    capture->release();
}

template <typename T>
void Analyzer::writeToFile(std::vector<T> _vec, std::filesystem::path _filename, std::string _type, std::string _extension)
{
    std::filesystem::path filepath = _filename.parent_path().string() + "/" + _filename.stem().string() + "-" + _type + _extension;
    std::ofstream outfile(filepath);
    for (size_t i = 0; i < _vec.size(); ++i) {
        outfile << i << ";" << _vec[i] << "\n";
    }
    outfile.close();
}

template <typename T>
void Analyzer::writeToFile(T _elem, std::filesystem::path _filename, std::string _type, std::string _extension)
{
    std::filesystem::path filepath = _filename.parent_path().string() + "/" + _filename.stem().string() + "-" + _type + _extension;
    std::ofstream outfile(filepath);
    outfile << 0 << ";" << _elem << "\n";
    outfile.close();
}

template <typename T>
T Analyzer::getMedian(std::vector<T> a)
{
    size_t n = a.size();
    if (n % 2 == 0)
    {
        std::nth_element(a.begin(), a.begin() + n / 2, a.end());
        std::nth_element(a.begin(), a.begin() + (n - 1)/2, a.end());
        return (T)(a[(n-1)/2] + a[n/2]) / 2.0;
    }
    else
    {
        std::nth_element(a.begin(), a.begin() + n/2, a.end());
        return (T)a[n/2];
    }
}

cv::Rect Analyzer::enlargeRect(cv::Rect _rect, double _factor)
{
    if(_factor < 0)
    {
        std::cerr << "Error:Something wrong with rectangle scaling factor\n";
        log_file << "Error:Something wrong with rectangle scaling factor\n";
    }
    int pre_width = _rect.width;
    int pre_height = _rect.height;
    _rect.width *= _factor;
    _rect.height *= _factor;
    _rect.x -= (_rect.width-pre_width)/2;
    _rect.y -= (_rect.height-pre_height)/2;
    return _rect;
}