//
// Created by nicholas on 25.05.24.
//

#ifndef MPEMBAGUI_RUNPROPERTIESWIDGET_H
#define MPEMBAGUI_RUNPROPERTIESWIDGET_H
#include <QWidget>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCalendarWidget>
#include "Measurement.h"

class RunPropertiesWidget : public QWidget
{
Q_OBJECT
public:
    explicit RunPropertiesWidget(QWidget *parent = 0);

    QFormLayout *layout;

    QLineEdit *flow_oil_edit;
    QLineEdit *flow_water_edit;
    QLineEdit *initial_temp_edit;
    QLineEdit *cooler_temp_edit;
    QCalendarWidget *date_recorded_edit;
    void setFromMeasurement(const Measurement& measurement) const;
    void writeToMeasurement(Measurement& measurement) const;

signals:
public slots:

};

#endif //MPEMBAGUI_RUNPROPERTIESWIDGET_H
