//
// Created by nicholas on 25.05.24.
//

#include "RunPropertiesWidget.h"
RunPropertiesWidget::RunPropertiesWidget(QWidget *parent) : QWidget(parent)
{
    layout = new QFormLayout();

    flow_oil_edit = new QLineEdit();
    flow_water_edit = new QLineEdit();
    initial_temp_edit = new QLineEdit();
    cooler_temp_edit = new QLineEdit();
    date_recorded_edit = new QCalendarWidget();

    layout->addRow(tr("&Oil flow rate:"), flow_oil_edit);
    layout->addRow(tr("&Water flow rate:"), flow_water_edit);
    layout->addRow(tr("&Warm temperature:"), initial_temp_edit);
    layout->addRow(tr("&Cold temperature:"), cooler_temp_edit);
    //layout->addRow(tr("&Recording date:"), date_recorded_edit);
    //Can be enabled if a date needs to be set (is written to case files)
    setLayout(layout);
}

void RunPropertiesWidget::setFromMeasurement(const Measurement &measurement) const
{
    flow_oil_edit->setText(QString::number(measurement.flow_rate_oil_ul));
    flow_water_edit->setText(QString::number(measurement.flow_rate_water_ul));
    initial_temp_edit->setText(QString::number(measurement.initial_temperature));
    cooler_temp_edit->setText(QString::number(measurement.cooler_temperature));
    date_recorded_edit->setSelectedDate(measurement.date);
}

void RunPropertiesWidget::writeToMeasurement(Measurement &measurement) const
{
    measurement.date = date_recorded_edit->selectedDate();
    measurement.flow_rate_oil_ul = flow_oil_edit->text().toDouble();
    measurement.flow_rate_water_ul = flow_water_edit->text().toDouble();
    measurement.initial_temperature = initial_temp_edit->text().toDouble();
    measurement.cooler_temperature = cooler_temp_edit->text().toDouble();
}