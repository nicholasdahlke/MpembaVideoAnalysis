//
// Created by nicholas on 21.11.23.
//

#include "Analyzer.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <opencv2/bgsegm.hpp>
#include <opencv2/intensity_transform.hpp>

Analyzer::Analyzer(std::string _filename)
{
    if (_filename.empty())
    {
        std::cerr << "Error:empty filename" << std::endl;
    }
    else
    {
        filename = _filename;
    }
}

int Analyzer::openCapture()
{
    if(filename.empty())
    {
        std::cerr << "Error: Filename empty" << std::endl;
        return -1;
    }

    capture = new cv::VideoCapture(filename);
    if(!capture->isOpened())
    {
        std::cerr << "Error: Unable to open " << filename << std::endl;
        return -2;
    }

    video_height = capture->get(cv::CAP_PROP_FRAME_HEIGHT);
    video_width = capture->get(cv::CAP_PROP_FRAME_WIDTH);
    video_frame_count = capture->get(cv::CAP_PROP_FRAME_COUNT);
    if(video_frame_count <= 0)
    {
        std::cerr << "Error: No frames found (Frame count is 0)" << std::endl;
        return -1;
    }

    std::cout << "Video with dimensions (w x h):" << video_width << "x" << video_height << std::endl;
    return 0;
}

int Analyzer::getDropletsFromVideo()
{
    std::shared_ptr<cv::BackgroundSubtractor> pBackSub;
    pBackSub = cv::createBackgroundSubtractorKNN(500, 400.0, false);
    cv::Mat frame_uncropped;
    cv::Mat fgMask;

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

    double ellipse_area = 0.0;
    int area_samples = 1;
    for(;;)
    {
        *capture >> frame_uncropped;
        if (frame_uncropped.empty())
            break;

        int frame_number = capture->get(cv::CAP_PROP_POS_FRAMES);
        double progress = ((float)frame_number/(float)video_frame_count)*100;
        std::cout << "Processing progress is " << progress << "%\n";

        cv::Mat frame = frame_uncropped(cv::Rect(0,0,video_width, video_height-20)); // Crop video
        cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
        pBackSub->apply(frame, fgMask); // Get foreground mask

        // Process mask
        cv::Mat mask_processed;
        cv::threshold(fgMask, mask_processed, 2, 255, cv::THRESH_BINARY);
        cv::morphologyEx(mask_processed, mask_processed, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5)));
        //cv::morphologyEx(mask_processed, mask_processed, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(25, 50)));

        // Find contours
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask_processed, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0,0));

        std::vector<cv::RotatedRect> minEllipses;
        for(std::vector<cv::Point> contour : contours)
        {
            if (contour.size() > 5)
            {
                cv::RotatedRect ellipse = cv::fitEllipse(contour);
                if(ellipse.size.height < video_height && ellipse.size.width < video_width)
                {
                    area_samples++;
                    ellipse_area = ellipse_area*(area_samples-1)/area_samples + (ellipse.size.height*ellipse.size.width)/area_samples;
                    std::cout << "Area average" << ellipse_area << "\n";
                    minEllipses.push_back(ellipse);
                }
            }

        }

        if(config.show_frames_droplets)
        {
            cv::cvtColor(frame, frame, cv::COLOR_GRAY2BGR);
            cv::cvtColor(fgMask, fgMask, cv::COLOR_GRAY2BGR);
            cv::cvtColor(mask_processed, mask_processed, cv::COLOR_GRAY2BGR);


            cv::Scalar blue = cv::Scalar(255, 0, 0);
            cv::Scalar red = cv::Scalar (0, 0, 255);
            cv::Scalar green = cv::Scalar (0, 255, 0);

            if (frame_number < config.skip_frames_droplets)
                cv::putText(frame, "Skipped", cv::Point(video_width/2-20, video_height/2), cv::FONT_HERSHEY_SIMPLEX, 3.0, red, 1);

            for(cv::RotatedRect ellipse : minEllipses)
            {

                cv::ellipse(frame, ellipse, blue, 2);
                cv::circle(frame, ellipse.center, 5, blue, -1);

            }

            cv::drawContours(frame, contours, -1, green);

            cv::Mat display_image = cv::Mat::zeros(cv::Size(frame.cols, frame.rows + fgMask.rows + mask_processed.rows), CV_8UC3);
            cv::Rect sub_roi = cv::Rect(0, 0, frame.cols, frame.rows);

            frame.copyTo(display_image(sub_roi));

            sub_roi.y = frame.rows;
            fgMask.copyTo(display_image(sub_roi));

            sub_roi.y = frame.rows + fgMask.rows;
            mask_processed.copyTo(display_image(sub_roi));

            cv::imshow("Processing Overview", display_image);
            cv::setWindowProperty("Processing Overview", 1, cv::WINDOW_NORMAL);

            int keyboard = cv::waitKey(200);
            if (keyboard == 'q' || keyboard == 27)
                break;
        }

        if (frame_number >= config.skip_frames_droplets)
            droplet_ellipses.push_back(minEllipses);
    }
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::cout << "Time per frame:" << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() / (double)video_frame_count << "ms" << std::endl;
    return 0;
}

int Analyzer::getDisplacementVectors()
{
    for (size_t i = 0; i < droplet_ellipses.size() - 1; i++)
    {
        std::cout << "Calculating displacement vectors for frame " << i << "\n";
        cv::Mat preview_image = cv::Mat::zeros(cv::Size(video_width, video_height), CV_8UC3);
        std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>> displacement_center;

        for(cv::RotatedRect ellipse : droplet_ellipses[i])
        {
            cv::ellipse(preview_image, ellipse, cv::Scalar (0, 255, 0), 2);
            cv::circle(preview_image, ellipse.center, 5, (0, 0, 255), -1);

            cv::Point curr_point = ellipse.center;
            std::vector<std::array<double, 3>> displacementVectors; // x-vector component, y-vector component, distance
            for(cv::RotatedRect ellipse_next : droplet_ellipses[i + 1])
            {
                std::array<double, 3> displacement{0, 0};
                if((ellipse_next.center.x - curr_point.x) > 0 && (ellipse_next.center.x - curr_point.x) < config.max_movement_threshold_displacement) { // Allow only positive displacement lower than the maximum allowed movement
                    displacement[0] = ellipse_next.center.x - curr_point.x;
                    displacement[1] = ellipse_next.center.y - curr_point.y;
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
                displacement_center.push_back({ellipse, displacementVectors.back()});
            }

        }

        if(config.show_frames_displacement)
        {
            for(cv::RotatedRect ellipse : droplet_ellipses[i + 1])
            {
                cv::ellipse(preview_image, ellipse, cv::Scalar (255, 0, 0), 2);
                cv::circle(preview_image, ellipse.center, 5, (0, 0, 255), -1);
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

    std::vector<std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>>>::iterator iter = displacement_vectors.begin();
    std::vector<cv::Point_<float>> visited;
    std::advance(iter, config.skip_frames_tracking);
    int frame_number = 0;
    while (iter != displacement_vectors.end())
    {
        std::cout << "Tracking for frame " << frame_number << "\n";
        frame_number++;
        for(std::tuple<cv::RotatedRect, std::array<double, 3>> displacement : *iter)
        {
            if(std::find(visited.begin(), visited.end(), std::get<0>(displacement).center) == visited.end())
            {
                std::vector<cv::Point_<float>> droplet_track;
                droplet_track.push_back(std::get<0>(displacement).center);
                std::vector<std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>>>::iterator next_iter =
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
    cv::Mat preview_image = cv::Mat::zeros(cv::Size(video_width, video_height), CV_8UC3);
    return 0;
}

int Analyzer::getVolumeFromDroplets()
{
    std::vector<std::vector<cv::RotatedRect>>::iterator frame_iter = droplet_ellipses.begin();
    std::advance(frame_iter, config.skip_frames_volume);
    int framenumber = config.skip_frames_volume;
    while(frame_iter != droplet_ellipses.end())
    {
        std::cout << "Analyzing frame " << framenumber << " with " << frame_iter->size() << " droplets\n";
        for(cv::RotatedRect droplet : *frame_iter)
        {
            if (droplet.center.x > config.left_border_volume && droplet.center.x < config.right_border_volume)
            {
                volumes.push_back(getVolumeFromDroplet(droplet, config.calib));
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
    for(std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>> a : displacement_vectors)
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
                              + "skip_frames_droplets" + std::to_string(_conf.skip_frames_droplets) + "\n"
                              + "show_frames_displacement=" + boolToString(_conf.show_frames_displacement) + "\n"
                              + "max_movement_threshold_displacement=" + std::to_string(_conf.max_movement_threshold_displacement) + "\n"
                              + "show_frames_tracking=" + boolToString(_conf.show_frames_tracking) + "\n"
                              + "skip_frames_tracking=" + std::to_string(_conf.skip_frames_tracking) + "\n"
                              + "skip_frames_volume=" + std::to_string(_conf.skip_frames_volume) + "\n"
                              + "left_border_volume=" + std::to_string(_conf.left_border_volume) + "\n"
                              + "right_border_volume=" + std::to_string(_conf.right_border_volume) + "\n"
                              + "x_threshold_count=" + std::to_string(_conf.x_threshold_count) + "\n"
                              + "calib=" + std::to_string(_conf.calib) + "\n";
    std::cout << config_string;
}

void Analyzer::printInfo()
{
    std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %X");
    std::string info_string = "time=" + ss.str() + "\n"
                              + "file=" + filename + "\n"
                              + "width=" + std::to_string(video_width) + "\n"
                              + "height=" + std::to_string(video_height) + "\n"
                              + "frames=" + std::to_string(video_frame_count) + "\n";
    std::cout << info_string;
    printConfig(config);
}

inline std::string Analyzer::boolToString(bool b)
{
    return b ? "Yes" : "No";
}
int Analyzer::analyze()
{
    if (configured)
    {
        int error_code = 0;

        error_code = openCapture();
        if(error_code != 0)
            return -1;

        error_code = getDropletsFromVideo();
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

        if(error_code != 0)
        {
            std::cerr << "Error: A processing error has occured" << std::endl;
            return -2;
        }
    }
    else
    {
        std::cerr << "Error: not configured" << std::endl;
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

const std::vector<double> Analyzer::getVolumes()
{
    return volumes;
}

int Analyzer::getNumDroplets()
{
    return num_droplets;
}
