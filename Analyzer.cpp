//
// Created by nicholas on 21.11.23.
//

#include "Analyzer.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <cmath>
#include <opencv2/bgsegm.hpp>
#include <opencv2/intensity_transform.hpp>
#include <opencv2/photo.hpp>


Analyzer::Analyzer(const std::filesystem::path& _filename, const std::filesystem::path& _net_filename)
{
    log_file.open(_filename.parent_path().string() + "/" + _filename.stem().string() + ".log", std::fstream::out | std::fstream::app);
    volume_images_path = _filename.parent_path().string() + "/" + _filename.stem().string() + "-volume-images/";
    std::filesystem::create_directories(volume_images_path);
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
    dnn_classes.emplace_back("droplets_frozen");

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
    int volume_image_nr = 0;
    for (size_t i = 0; i < _num_droplets; i++)
    {
        cv::Mat current_frame;
        *capture >> current_frame;
        if(current_frame.empty())
            break;
        std::vector<cv::Mat> current_detections = dnn_results[i];
        cv::Mat current_annotated = current_frame.clone();
        std::vector<Detection> detections = getBoundingRectFromResults(current_annotated, current_detections, dnn_classes);
        cv::cvtColor(current_frame, current_frame, cv::COLOR_BGR2GRAY);
        std::vector<Droplet> ellipses;
        int volume_droplet_nr = 0;
        for (Detection detection_obj : detections)
        {
            cv::Rect detection = enlargeRect(detection_obj.getRect(),
                                             1.2); //Enlarge rectangle to avoid problematic collisions with borders
            cv::rectangle(current_annotated, detection, cv::Scalar(0, 255, 0), 2);
            bool skip_droplet = false;
            {
                // Check if droplet is inside borders
                if (detection.x < config.left_border_volume)
                    skip_droplet = true;
                if (detection.y < 0)
                    skip_droplet = true;
                if (detection.x + detection.width > video_width)
                    skip_droplet = true;
                if (detection.y + detection.height > video_height)
                    skip_droplet = true;
            }
            {
                // Check for intersecting droplets
                for (Detection detection1 : detections)
                {
                    if(detection1 != detection_obj)
                    {
                        if ((enlargeRect(detection1.getRect(), 1.2) & detection).area() > 0)
                        {
                            skip_droplet = true;
                            std::cout << "Skipping droplets because it intersects another" << "\n";
                            log_file << "Skipping droplets because it intersects another" << "\n";
                        }
                    }
                }
            }

            int x_offset = detection.x;
            int y_offset = detection.y;
            if (!skip_droplet)
            {
                std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
                cv::Mat droplet_roi = current_frame(detection); // Get detection ROI from image
                cv::fastNlMeansDenoising(droplet_roi, droplet_roi, 10, 7, 21); // Denoise image
                cv::Mat grabcut_input;
                float downsampling_factor = 0.5;
                cv::resize(droplet_roi, grabcut_input, cv::Size(0, 0), downsampling_factor, downsampling_factor); // Downscale image for faster grabcut performance

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

                std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

                std::cout << "Applied grabcut algorithm on frame " << volume_image_nr << " droplet " << volume_droplet_nr << " in " <<std::chrono::duration_cast<std::chrono::milliseconds>(t2 -t1).count() << "ms\n";
                log_file << "Applied grabcut algorithm on frame " << volume_image_nr << " droplet " << volume_droplet_nr << " in " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 -t1).count() << "ms\n";

                if (!contours.empty())
                {
                    // Find largest contour
                    std::vector<cv::Point> contour = *std::max_element(contours.begin(),
                                                                       contours.end(),
                                                                       [](const std::vector<cv::Point> &a,
                                                                          const std::vector<cv::Point> &b) -> bool {
                                                                           return cv::contourArea(a) <
                                                                                  cv::contourArea(b);
                                                                       });

                    if(contour.size() > 5) // Check if enough points are present
                    {
                        // Fit an ellipse
                        cv::RotatedRect ellipse_fit = cv::fitEllipse(contour);

                        // Shift the ellipse back to the original position
                        ellipse_fit.center.x += x_offset;
                        ellipse_fit.center.y += y_offset;

                        ellipses.emplace_back(ellipse_fit, true, detection_obj.getDetectionType() == dnn_classes[1]);
                    }
                    else
                    {
                        skip_droplet = true;
                    }
                }
                else
                {
                    skip_droplet = true;
                }
            }

            if (skip_droplet)
            {
                int width = detection.width;
                int height = detection.height;
                cv::Point center(x_offset + width/2, y_offset + height/2);
                ellipses.emplace_back(cv::RotatedRect(center, cv::Size(width, height), 0), false, detection_obj.getDetectionType() == dnn_classes[1]);

            }
        }
        if(config.show_frames_droplets)
        {
            for(Droplet droplet : ellipses)
            {
                cv::Scalar color(0, 0, 255);
                if(droplet.getCalcVolume())
                    color = cv::Scalar(0, 255, 0);
                cv::ellipse(current_annotated, droplet.getEllipse(), color);
            }
            cv::imshow("Frames", current_annotated);
            cv::waitKey(1);
            cv::imwrite(volume_images_path.string() + "annotated_" + std::to_string(volume_image_nr) + "_" + std::to_string(volume_droplet_nr) + ".jpg", current_annotated);
        }
        droplet_ellipses.push_back(ellipses);
        volume_image_nr++;
    }
    capture->set(cv::CAP_PROP_POS_FRAMES, 0); // Reset frame pointer to beginning
    return 0;
}


int Analyzer::getDisplacementVectors()
{
    for (size_t i = 0; i < droplet_ellipses.size() - 1; i++)
    {
        std::cout << "Calculating displacement vectors for frame " << i << "\n";
        log_file << "Calculating displacement vectors for frame " << i << "\n";

        cv::Mat preview_image = cv::Mat::zeros(cv::Size(video_width, video_height), CV_8UC3);
        std::vector<Displacement> displacement_center;

        for(Droplet droplet : droplet_ellipses[i])
        {
            cv::ellipse(preview_image, droplet.getEllipse(), cv::Scalar (0, 255, 0), 2);
            cv::circle(preview_image, droplet.getEllipse().center, 5, (0, 0, 255), -1);

            cv::Point curr_point = droplet.getEllipse().center;
            if(curr_point.x > config.right_border_displacement)
            {
                std::cerr << "Droplet behind border, displacement calculation will most likely fail:SKIPPING" << std::endl;
                log_file << "Droplet behind border, displacement calculation will most likely fail:SKIPPING" << "\n";
            }
            else
            {
                std::vector<std::tuple<std::array<double, 3>, Droplet>> displacementVectors; // x-vector component, y-vector component, distance
                for(Droplet ellipse_next : droplet_ellipses[i + 1])
                {
                    std::array<double, 3> displacement{0};
                    if((ellipse_next.getEllipse().center.x - curr_point.x) > 0 && (ellipse_next.getEllipse().center.x - curr_point.x) < config.max_movement_threshold_displacement) { // Allow only positive displacement lower than the maximum allowed movement
                        displacement[0] = ellipse_next.getEllipse().center.x - curr_point.x;
                        displacement[1] = ellipse_next.getEllipse().center.y - curr_point.y;
                        displacement[2] = sqrt(pow(displacement[0], 2) + pow(displacement[1], 2));
                        displacementVectors.emplace_back(displacement, ellipse_next);
                    }
                }

                if(!displacementVectors.empty())
                {
                    std::sort(displacementVectors.begin(), displacementVectors.end(), [&](std::tuple<std::array<double, 3>, Droplet> & a, std::tuple<std::array<double, 3>, Droplet> & b)->bool{return std::get<0>(a)[2]>std::get<0>(b)[2];});
                    cv::Point displacedPoint;
                    displacedPoint.x = curr_point.x + std::get<0>(displacementVectors.back())[0];
                    displacedPoint.y = curr_point.y + std::get<0>(displacementVectors.back())[1];

                    cv::line(preview_image, curr_point, displacedPoint, cv::Scalar(0, 0, 255), 1);
                    displacement_center.emplace_back(droplet, std::get<1>(displacementVectors.back()), std::get<0>(displacementVectors.back()), i);
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
                cv::ellipse(preview_image, droplet.getEllipse(), cv::Scalar (255, 0, 0), 2);
                cv::circle(preview_image, droplet.getEllipse().center, 5, (0, 0, 255), -1);
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
            if (droplet.getEllipse().center.x > config.left_border_volume && droplet.getEllipse().center.x < config.right_border_volume && droplet.getCalcVolume())
            {
                volumes.push_back(getVolumeFromDroplet(droplet.getEllipse(), config.calib));
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

    double c = std::max(_droplet.size.width, _droplet.size.height) / 2.0;
    double a = std::min(_droplet.size.width, _droplet.size.height) / 2.0;
    a = a*_calib;
    c = c*_calib;
    return (4.0/3.0) * std::numbers::pi * std::pow(a, 2) * c;
}

int Analyzer::countDroplets()
{
    std::vector<Displacement> total_displacements;
    for(const std::vector<Displacement>& a : displacement_vectors)
    {
        for(Displacement b : a)
        {
            total_displacements.push_back(b);
        }
    }
    for(Displacement displacement : total_displacements)
    {
        if(displacement.droplet.getEllipse().center.x < config.x_threshold_count &&  displacement.droplet_next.getEllipse().center.x > config.x_threshold_count && !(displacement.droplet.getIsFrozen() || displacement.droplet_next.getIsFrozen()))
            num_droplets++;

        if(displacement.droplet.getEllipse().center.x < config.x_threshold_count &&  displacement.droplet_next.getEllipse().center.x > config.x_threshold_count && (displacement.droplet.getIsFrozen() || displacement.droplet_next.getIsFrozen()))
            num_droplets_frozen++;
    }

    return 0;
}

int Analyzer::measureInterDropletDistances()
{
    std::vector<Displacement> total_displacements;
    for(const std::vector<Displacement>& a : displacement_vectors)
    {
        for(Displacement b : a)
        {
            total_displacements.push_back(b);
        }
    }
    std::vector<Displacement> crossings;
    for (Displacement displacement : total_displacements) // Measure only when crossing to avoid double counting a distance
    {
        if(displacement.droplet.getEllipse().center.x < config.x_threshold_count &&  displacement.droplet_next.getEllipse().center.x > config.x_threshold_count)
            crossings.push_back(displacement);
    }
    std::vector<double> distances_m;
    for(auto it = crossings.begin(); std::next(it) != crossings.end(); std::advance(it, 1))
    {
        double d1 = it->droplet_next.getEllipse().center.x - config.x_threshold_count;
        double d2 = std::next(it)->droplet_next.getEllipse().center.x - config.x_threshold_count;
        double v = 0.5*(it->vector[2] + std::next(it)->vector[2]);
        double dt = std::next(it)->start_frame_number - it->start_frame_number;
        double l = v*dt - d2 + d1;
        distances_m.push_back(l * config.calib);
    }
    distances = distances_m;
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
    std::stringstream info_stream;
    info_stream << std::scientific;
    info_stream << "show_frames_droplets=" + boolToString(_conf.show_frames_droplets) + "\n";
    info_stream << "show_frames_displacement=" + boolToString(_conf.show_frames_displacement) + "\n";
    info_stream << "show_displacement_vectors" + boolToString(_conf.show_displacement_vectors) + "\n";
    info_stream << "right_border_displacement=" << _conf.right_border_displacement << "\n";
    info_stream << "max_movement_threshold_displacement=" << _conf.max_movement_threshold_displacement << "\n";
    info_stream << "skip_frames_volume=" << _conf.skip_frames_volume << "\n";
    info_stream << "left_border_volume=" << _conf.left_border_volume << "\n";
    info_stream << "right_border_volume=" << _conf.right_border_volume << "\n";
    info_stream << "x_threshold_count=" << _conf.x_threshold_count << "\n";
    info_stream <<  "calib=" << _conf.calib << "\n";
    info_stream << "score_threshold=" << _conf.score_threshold << "\n";
    info_stream << "nms_threshold=" << _conf.nms_threshold << "\n";
    info_stream << "confidence_threshold" << _conf.confidence_threshold << "\n";
    info_stream << "speed_border_left" << _conf.speed_border_left << "\n";
    info_stream << "speed_border_right" << _conf.speed_border_right << "\n";


    std::cout << info_stream.str();
    log_file << info_stream.str();
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
        applyNetToFrames(_num_droplets);

        getDropletsFromVideo(_num_droplets);
        getDisplacementVectors();
        getVolumeFromDroplets();
        countDroplets();
        measureInterDropletDistances();
        getSpeeds();

        if (config.show_frames_displacement)
            showAllMovementVectors();

        std::cout << "Analysis finished, writing results to file" << std::endl;
        log_file << "Analysis finished, writing results to file" << "\n";
        writeToFile(volumes, filename, "volumes", ".csv");
        writeToFile(num_droplets, filename, "droplet_count", ".csv");
        writeToFile(num_droplets_frozen, filename, "droplet_count_frozen", ".csv");
        writeToFile(distances, filename, "distances", ".csv");
        writeToFile(speeds, filename, "speeds", ".csv");
        writeToFile(config.calib, filename, "calibration", ".csv");
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
    volumes.clear();
    num_droplets = 0;
}

int Analyzer::getNumDroplets() const
{
    return num_droplets + num_droplets_frozen;
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

std::vector<Detection> Analyzer::getBoundingRectFromResults(cv::Mat & _annotation_image, std::vector<cv::Mat> & outputs, const std::vector<std::string> & _class_name, float _size) const
{
    std::vector<int> class_ids;
    std::vector<float> confidences;
    std::vector<cv::Rect> boxes;

    float x_factor = _annotation_image.cols / _size;
    float y_factor = _annotation_image.rows / _size;
    float *data = (float *)outputs[0].data;
    const int rows = 25200;
    const int dimensions = 7; // Always adjust for multi class models

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

                boxes.emplace_back(cv::Point(left, top), cv::Point(left + width, top + height));
            }
        }
        data += dimensions;
    }
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, config.score_threshold, config.nms_threshold, indices);
    std::vector<Detection> bounding_rects;
    for (int i = 0; i < indices.size(); i++)
    {
        int idx = indices[i];
        cv::Rect box = boxes[idx];
        int left = box.x;
        int top = box.y;

        //cv::Rect current_bounding_rect = box;
        cv::rectangle(_annotation_image, box, cv::Scalar(255, 0, 0), 3);
        // Get the label for the class name and its confidence.
        std::string label = cv::format("%.2f", confidences[idx]);
        label = _class_name[class_ids[idx]] + ":" + label;
        // Draw class labels.
        drawLabel(_annotation_image, label, left, top);
        bounding_rects.emplace_back(box, _class_name[class_ids[idx]], confidences[idx]);

    }
    return bounding_rects;
}
void Analyzer::showAllMovementVectors()
{
    cv::RNG rng;
    cv::Mat preview_image;
    *capture >> preview_image;
    for(std::vector<Displacement> image_displ : displacement_vectors)
    {
        for (Displacement displ_vec : image_displ)
        {
            cv::Point curr_point = displ_vec.droplet.getEllipse().center;
            std::array<double, 3> displ_vec_array = displ_vec.vector;
            cv::Point displaced_point;
            displaced_point.x = curr_point.x + displ_vec_array[0];
            //int rand_shift = rng.uniform(-200,  200);  // Randomly shift the movement vectors up and down for better visibility
            int rand_shift = 0;
            displaced_point.y = curr_point.y + displ_vec_array[1] + rand_shift;
            curr_point.y = curr_point.y + rand_shift;
            cv::line(preview_image, curr_point, displaced_point, cv::Scalar(rng.uniform(0,255), rng.uniform(0,rng.uniform(0,255)), 255), 1);
        }
    }
    cv::imshow("Displacement Vectors", preview_image);
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

int Analyzer::getSpeeds()
{   //Get the droplet speed in m/frame
    if(displacement_vectors.empty())
    {
        std::cerr << "Error: No displacement vectors calculated\n";
        log_file << "Error: No displacement vectors calculated\n";
        return -1;
    }
    for(std::vector<Displacement> disp_vect_image : displacement_vectors)
    {
        for(Displacement displacement : disp_vect_image)
        {
            //if(displacement.droplet.getEllipse().center.x < config.speed_border_left || displacement.droplet_next.getEllipse().center.x > (video_width - config.speed_border_right))
                speeds.push_back(displacement.vector[2]*config.calib);
        }
    }
    return 0;
}