//
// Created by nicholas on 25.05.24.
//

#include "SetupPropertiesWidget.h"
SetupPropertiesWidget::SetupPropertiesWidget(QWidget *parent) : QWidget(parent)
{
    layout = new QFormLayout();

    fps_edit = new QLineEdit();
    cooler_length_edit = new QLineEdit();
    thermal_conductivity_tubing_edit = new QLineEdit();
    inner_radius_tubing_edit = new QLineEdit();
    outer_radius_tubing_edit = new QLineEdit();
    medium_density_edit = new QLineEdit();

    layout->addRow(tr("&FPS:"), fps_edit);
    layout->addRow(tr("&Cooler length:"), cooler_length_edit);
    layout->addRow(tr("&Thermal conductivity tubing:"), thermal_conductivity_tubing_edit);
    layout->addRow(tr("&Inner radius tubing:"), inner_radius_tubing_edit);
    layout->addRow(tr("&Outer radius tubing:"), outer_radius_tubing_edit);
    layout->addRow(tr("&Medium density:"), medium_density_edit);

    setLayout(layout);
}

void SetupPropertiesWidget::setFromMeasurement(const Measurement& measurement) const
{
    fps_edit->setText(QString::number(measurement.frames_per_second));
    cooler_length_edit->setText(QString::number(measurement.cooler_length));
    thermal_conductivity_tubing_edit->setText(QString::number(measurement.thermal_conductivity_tubing));
    inner_radius_tubing_edit->setText(QString::number(measurement.inner_radius_tubing));
    outer_radius_tubing_edit->setText(QString::number(measurement.outer_radius_tubing));
    medium_density_edit->setText(QString::number(measurement.water_density));
}

void SetupPropertiesWidget::writeToMeasurement(Measurement &measurement) const
{
    measurement.frames_per_second = fps_edit->text().toDouble();
    measurement.cooler_length = cooler_length_edit->text().toDouble();
    measurement.thermal_conductivity_tubing = thermal_conductivity_tubing_edit->text().toDouble();
    measurement.inner_radius_tubing = inner_radius_tubing_edit->text().toDouble();
    measurement.outer_radius_tubing = outer_radius_tubing_edit->text().toDouble();
    measurement.water_density = medium_density_edit->text().toDouble();
}