#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/video.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <array>

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

void trackDroplet(std::vector<std::vector<cv::RotatedRect>> _droplets, int _width, int _height)
{
    for (size_t i = 0; i < _droplets.size() - 1; i++)
    {
        cv::Mat preview_image = cv::Mat::zeros(cv::Size(_width, _height), CV_8UC3);
        for(cv::RotatedRect ellipse : _droplets[i])
        {

            cv::ellipse(preview_image, ellipse, cv::Scalar (0, 255, 0), 2);
            cv::circle(preview_image, ellipse.center, 5, (0, 0, 255), -1);

            cv::Point curr_point = ellipse.center;
            std::vector<std::array<double, 3>> displacementVectors; // x-vector component, y-vector component, distance
            for(cv::RotatedRect ellipse : _droplets[i+1])
            {
                std::array<double, 3> displacement{0, 0};
                displacement[0] = ellipse.center.x - curr_point.x;
                displacement[1] = ellipse.center.y - curr_point.y;
                displacement[2] = sqrt(pow(displacement[0], 2) + pow(displacement[1], 2));
                displacementVectors.push_back(displacement);
            }
            if(!displacementVectors.empty())
            {
                std::sort(displacementVectors.begin(), displacementVectors.end(), [&](std::array<double, 3> & a, std::array<double, 3> & b)->bool{return a[2]>b[2];});
                cv::Point displacedPoint;
                displacedPoint.x = curr_point.x + displacementVectors.back()[0];
                displacedPoint.y = curr_point.y + displacementVectors.back()[1];

                cv::line(preview_image, curr_point, displacedPoint, cv::Scalar(0, 0, 255), 1);
            }

        }

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
}

std::vector<std::vector<double>> getVolumeFromEllipses(std::vector<std::vector<cv::RotatedRect>> _ellipses)
{
    // The shape of the droplet is approximated as an elongated rotational ellipsoid
    std::vector<std::vector<double>> volumes;
    for(std::vector<cv::RotatedRect> frame : _ellipses)
    {
        std::vector<double> volumes_frame;
        for(cv::RotatedRect ellipse : frame)
        {
            double a;
            double c;
            if (ellipse.size.width < ellipse.size.height)
            {
                a = ellipse.size.width / 2;
                c = ellipse.size.height / 2;
            }
            else
            {
                a = ellipse.size.height / 2;
                c = ellipse.size.width / 2;
            }
            double volume = (4.0/3.0) * std::numbers::pi * pow(a, 2) * c;
            volumes_frame.push_back(volume);
        }
        volumes.push_back(volumes_frame);
    }
    return volumes;
}


int main() {
    std::cout << "Mpemba Video Analysis" << std::endl;
    std::string videofile = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/videos/first_ice_ever_20231118_020835_20231118_021530.avi";
    std::string expfile = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/videos/export.exp";

    cv::VideoCapture capture(videofile);
    if(!capture.isOpened())
    {
        std::cerr << "Unable to open: " << videofile << std::endl;
    }

    int video_width = capture.get(cv::CAP_PROP_FRAME_WIDTH);
    int video_height = capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    std::cout << "Video with dimensions (w x h):" << video_width << "x" << video_height << std::endl;

    std::vector<std::vector<cv::RotatedRect>> dropletEllipses = getDropletsFromVideo(capture, video_width, video_height,
                                                                                     false);
    trackDroplet(dropletEllipses, video_width, video_height);
    std::vector<std::vector<double>> volumes = getVolumeFromEllipses(dropletEllipses);

    std::ofstream outfile("/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/videos/out.csv");
    outfile << "frame;volume" << std::endl;
    for (size_t i = 0; i < volumes.size(); ++i) {
        for(double volume : volumes[i])
            outfile << i << ";" << volume << "\n";
    }
    outfile.close();

    return 0;
}
