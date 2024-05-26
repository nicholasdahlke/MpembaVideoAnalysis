//
// Created by nicholas on 25.05.24.
//

#include <iostream>
#include "Measurement.h"

Measurement::Measurement(QString filename)
{
    this->filename = filename;
}

void Measurement::generateCaseFile()
{
    std::filesystem::path video_path(filename.toStdString());
    std::filesystem::path case_path = video_path.parent_path();
    case_path += "/";
    case_path += video_path.stem();
    case_path += ".cf";
    std::cout << case_path.string();
    std::fstream case_fstream(case_path, std::fstream::out);
    std::locale::global(std::locale::classic());
    if (case_fstream.is_open())
    {
        case_fstream << "[data]\n";

        case_fstream << "video = \"" << filename.toStdString() << "\"\n";
        case_fstream << "flow_rate_oil_ul = " << std::to_string(flow_rate_oil_ul) << "\n";
        case_fstream << "flow_rate_water_ul = " << std::to_string(flow_rate_water_ul) << "\n";
        case_fstream << "frames_per_second = " << std::to_string(frames_per_second) << "\n";
        case_fstream << "initial_temperature = " << std::to_string(initial_temperature) << "\n";
        case_fstream << "cooler_temperature = " << std::to_string(cooler_temperature) << "\n";
        case_fstream << "cooler_length = " << std::to_string(cooler_length) << "\n";
        case_fstream << "thermal_conductivity_tubing = " << std::to_string(thermal_conductivity_tubing) << "\n";
        case_fstream << "inner_radius_tubing = " << std::to_string(inner_radius_tubing) << "\n";
        case_fstream << "outer_radius_tubing = " << std::to_string(outer_radius_tubing) << "\n";
        case_fstream << "water_density = " << std::to_string(water_density) << "\n";
        case_fstream << "date_recorded = " << date.toString("yyyy-MM-dd '12:00:00'").toStdString() << "\n";
        case_fstream.close();
        case_file = QString::fromUtf8(case_path.c_str());
    }
}