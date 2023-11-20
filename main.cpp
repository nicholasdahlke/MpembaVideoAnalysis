#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/video.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <X11/cursorfont.h>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <array>
#include <set>
#include "Calibrator.h"

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
}

std::vector<std::vector<cv::RotatedRect>> getDropletsFromVideo(cv::VideoCapture _capture, int _width, int _height, bool _mark_frames)
{
    std::shared_ptr<cv::BackgroundSubtractor> pBackSub;
    pBackSub = cv::createBackgroundSubtractorKNN();

    cv::Mat frame_uncropped;
    cv::Mat fgMask;

    std::vector<std::vector<cv::RotatedRect>> dropletEllipses;

    int num_frames = _capture.get(cv::CAP_PROP_FRAME_COUNT);

    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    for(;;)
    {
        _capture >> frame_uncropped;
        if (frame_uncropped.empty())
            break;

        int frame_number = _capture.get(cv::CAP_PROP_POS_FRAMES);
        double progress = ((float)frame_number/(float)num_frames)*100;
        std::cout << "Processing progress is " << progress << "%\n";

        cv::Mat frame = frame_uncropped(cv::Rect(0,0,_width, _height-20)); // Crop video

        pBackSub->apply(frame, fgMask); // Get foreground mask

        // Process mask
        cv::Mat mask_processed;
        cv::threshold(fgMask, mask_processed, 3, 255, cv::THRESH_BINARY);
        cv::morphologyEx(mask_processed, mask_processed, cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));
        cv::morphologyEx(mask_processed, mask_processed, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(25, 50)));

        // Find contours
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask_processed, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0,0));

        std::vector<cv::RotatedRect> minEllipses;
        for(std::vector<cv::Point> contour : contours)
        {
            if (contour.size() > 5)
            {
                cv::RotatedRect ellipse = cv::fitEllipse(contour);
                if(ellipse.size.height < _height && ellipse.size.width < _height)
                    minEllipses.push_back(ellipse);
            }

        }

        if(_mark_frames)
        {
            for(cv::RotatedRect ellipse : minEllipses)
            {
                cv::Scalar blue = (255, 255, 255);
                cv::Scalar red = (255, 0, 255);
                cv::ellipse(frame, ellipse, blue, 2);
                cv::circle(frame, ellipse.center, 5, red, -1);

            }
            cv::imshow("Frame", frame);
            int keyboard = cv::waitKey(100);
            if (keyboard == 'q' || keyboard == 27)
                break;
        }

        dropletEllipses.push_back(minEllipses);
    }
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::cout << "Time per frame:" << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() / (double) num_frames << "ms" << std::endl;
    return dropletEllipses;
}

std::vector<std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>>> getDisplacementVectors(std::vector<std::vector<cv::RotatedRect>> _droplets, int _width, int _height, float max_movement_threshold, bool show_frames=false)
{
    std::vector<std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>>> result;
    for (size_t i = 0; i < _droplets.size() - 1; i++)
    {
        std::cout << "Calculating displacement vectors for frame " << i << "\n";
        cv::Mat preview_image = cv::Mat::zeros(cv::Size(_width, _height), CV_8UC3);
        std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>> displacement_center;

        for(cv::RotatedRect ellipse : _droplets[i])
        {
            cv::ellipse(preview_image, ellipse, cv::Scalar (0, 255, 0), 2);
            cv::circle(preview_image, ellipse.center, 5, (0, 0, 255), -1);

            cv::Point curr_point = ellipse.center;
            std::vector<std::array<double, 3>> displacementVectors; // x-vector component, y-vector component, distance
            for(cv::RotatedRect ellipse_next : _droplets[i+1])
            {
                std::array<double, 3> displacement{0, 0};
                if((ellipse_next.center.x - curr_point.x) > 0 && (ellipse_next.center.x - curr_point.x) < max_movement_threshold) { // Allow only positive displacement lower than the maximum allowed movement
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

        if(show_frames)
        {
            for(cv::RotatedRect ellipse : _droplets[i+1])
            {
                cv::ellipse(preview_image, ellipse, cv::Scalar (255, 0, 0), 2);
                cv::circle(preview_image, ellipse.center, 5, (0, 0, 255), -1);
            }
            cv::imshow("Frame", preview_image);
            int keyboard = cv::waitKey(100);
            if (keyboard == 'q' || keyboard == 27)
                break;
        }
        result.push_back(displacement_center);
    }
    return result;
}

std::vector<std::vector<cv::Point_<float>>> trackDroplet(std::vector<std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>>> _displacement_vectors,  int _width, int _height, int skip_frames=0, bool show_frames=false)
{
    std::vector<std::vector<cv::Point_<float>>> droplet_tracks;
    std::vector<std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>>>::iterator iter = _displacement_vectors.begin();
    std::vector<cv::Point_<float>> visited;
    std::advance(iter, skip_frames);
    int framenumber = 0;
    while (iter != _displacement_vectors.end())
    {
        std::cout << "Tracking for frame " << framenumber << "\n";
        framenumber++;
        for(std::tuple<cv::RotatedRect, std::array<double, 3>> displacement : *iter)
        {
            if(std::find(visited.begin(), visited.end(), std::get<0>(displacement).center) == visited.end())
            {
                std::vector<cv::Point_<float>> droplet_track;
                droplet_track.push_back(std::get<0>(displacement).center);
                std::vector<std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>>>::iterator next_iter =
                        iter + 1;
                while (!next_iter->empty() && next_iter != _displacement_vectors.end()) {
                    for (std::tuple<cv::RotatedRect, std::array<double, 3>> displacement: *next_iter) {
                        if (std::find(visited.begin(), visited.end(), std::get<0>(displacement).center) == visited.end())
                        {
                            int dx = droplet_track.back().x - std::get<0>(displacement).center.x;
                            int dy = droplet_track.back().y - std::get<0>(displacement).center.y;
                            if (dx == 0 && dy == 0) {
                                droplet_track.push_back(std::get<0>(displacement).center);
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
    cv::Mat preview_image = cv::Mat::zeros(cv::Size(_width, _height), CV_8UC3);
    return droplet_tracks;
}

double getVolumeFromDroplet(cv::RotatedRect _droplet)
{
    float c = std::max(_droplet.size.width, _droplet.size.height) / 2.0;
    float a = std::min(_droplet.size.width, _droplet.size.height) / 2.0;

    return (4.0/3.0) * std::numbers::pi * std::pow(a, 2) * c;
}

std::vector<double> getVolumeFromDroplets(std::vector<std::vector<cv::RotatedRect>> _droplets, int _left_border, int _right_border, int skipframes = 0)
{
    // The shape of the droplet is approximated as an elongated rotational ellipsoid V=(4/3)*pi*a^2*c#
    std::vector<std::vector<cv::RotatedRect>>::iterator frame_iter = _droplets.begin();
    std::advance(frame_iter, skipframes);
    std::vector<double> volumes;
    int framenumber = skipframes;
    while(frame_iter != _droplets.end())
    {
        std::cout << "Analyzing frame " << framenumber << " with " << frame_iter->size() << " droplets\n";
        for(cv::RotatedRect droplet : *frame_iter)
        {
            if (droplet.center.x > _left_border && droplet.center.x < _right_border)
            {
                volumes.push_back(getVolumeFromDroplet(droplet));
            }
        }
        ++frame_iter;
        framenumber++;
    }
    return volumes;

}

int countDroplets(std::vector<std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>>> _displacements, int _x_threshold)
{
    std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>> total_displacements;
    for(std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>> a : _displacements)
    {
        for(std::tuple<cv::RotatedRect, std::array<double, 3>> b : a)
        {
            total_displacements.push_back(b);
        }
    }
    int count = 0;
    for(std::tuple<cv::RotatedRect, std::array<double, 3>> displacement : total_displacements)
    {
        if(std::get<0>(displacement).center.x < _x_threshold && (std::get<0>(displacement).center.x + std::get<1>(displacement)[0]) > _x_threshold)
            count++;;
    }
    return count;
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


int main() {
    std::cout << "Mpemba Video Analysis" << std::endl;
    std::string videofile = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/videos/dropletvideo1.avi";
    std::string expfile = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/videos/export.exp";

    cv::VideoCapture capture(videofile);
    if(!capture.isOpened())
    {
        std::cerr << "Unable to open: " << videofile << std::endl;
    }

    int video_width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
    int video_height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    std::cout << "Video with dimensions (w x h):" << video_width << "x" << video_height << std::endl;

    Calibrator calib(capture, 0);

    /*std::vector<std::vector<cv::RotatedRect>> dropletEllipses = getDropletsFromVideo(capture, video_width, video_height,
                                                                                     false);
    std::vector<std::vector<std::tuple<cv::RotatedRect, std::array<double, 3>>>> displacement = getDisplacementVectors(dropletEllipses, video_width, video_height, 100,
                                                                                                                       false);
    std::vector<double> volumes = getVolumeFromDroplets(dropletEllipses, 50, 600, 10);


    writeToFile(volumes,"/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/videos/volumes.csv");
    int count = countDroplets(displacement, 100);
    std::cout << "Counted " << count << " droplets" << std::endl;
    */

    return 0;
}
