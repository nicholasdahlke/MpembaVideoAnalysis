//
// Created by nicholas on 25.05.24.
//

#include "FilePropertiesWidget.h"

FilePropertiesWidget::FilePropertiesWidget(QWidget *parent) : QWidget(parent)
{
    layout = new QFormLayout();

    filename_label = new QLabel("-");
    case_label = new QLabel("-");
    flip = new QCheckBox();

    layout->addRow(tr("&Filename:"), filename_label);
    layout->addRow(tr("&Case file:"), case_label);
    layout->addRow(tr("&Flip video:"), flip);

    setLayout(layout);
}

void FilePropertiesWidget::setFromMeasurement(const Measurement& measurement) const
{
    filename_label->setText(measurement.filename);
    case_label->setText(measurement.case_file);
    flip->setCheckState(measurement.flip_video);
}

void FilePropertiesWidget::writeToMeasurement(Measurement &measurement) const
{
    measurement.flip_video = flip->checkState();
}