#include <iostream>
#include <fstream>
#include "Calibrator.h"
#include "Analyzer.h"

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


int main(int argc, char* argv[]) {
    cv::CommandLineParser parser(argc, argv, params);
    parser.about("This program analyzes a video of droplets under a microscope.");
    if(parser.has("help"))
    {
        parser.printMessage();
    }



    std::cout << "Mpemba Video Analysis" << std::endl;
    std::string videofile = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/videos/fc2_save_2_2023-11-23-184941-0000.avi";
    std::string back_file = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/videos/background.avi";

    Analyzer analyzer(videofile, back_file);
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


    return 0;
}
