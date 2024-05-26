//
// Created by nicholas on 25.05.24.
//

#ifndef MPEMBAGUI_MEASUREMENT_H
#define MPEMBAGUI_MEASUREMENT_H
#include <string>
#include <QTableWidgetItem>
#include <QDate>
#include <memory>
#include <chrono>
#include <filesystem>
#include <fstream>

class Measurement
{
public:
    explicit Measurement(QString filename);

    QString filename;
    QString case_file = "-";
    double flow_rate_oil_ul = 0;
    double flow_rate_water_ul = 0;
    double frames_per_second = 0;
    double initial_temperature = 0;
    double cooler_temperature = 0;
    double cooler_length = 0;
    double thermal_conductivity_tubing = 0;
    double inner_radius_tubing = 0;
    double outer_radius_tubing = 0;
    double water_density = 0;
    Qt::CheckState flip_video = Qt::Unchecked;
    QDate date = QDate(2006, 12, 20);

    void generateCaseFile();

};


#endif //MPEMBAGUI_MEASUREMENT_H
