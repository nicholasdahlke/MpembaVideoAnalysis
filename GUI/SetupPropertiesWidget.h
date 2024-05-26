//
// Created by nicholas on 25.05.24.
//

#ifndef MPEMBAGUI_SETUPPROPERTIESWIDGET_H
#define MPEMBAGUI_SETUPPROPERTIESWIDGET_H
#include <QWidget>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include "Measurement.h"

class SetupPropertiesWidget : public QWidget
{
    Q_OBJECT
    public:
        explicit SetupPropertiesWidget(QWidget *parent = 0);

        QFormLayout *layout;

        QLineEdit *fps_edit;
        QLineEdit *cooler_length_edit;
        QLineEdit *thermal_conductivity_tubing_edit;
        QLineEdit *inner_radius_tubing_edit;
        QLineEdit *outer_radius_tubing_edit;
        QLineEdit *medium_density_edit;
        void setFromMeasurement(const Measurement& measurement) const;
        void writeToMeasurement(Measurement& measurement) const;

    signals:
    public slots:

};


#endif //MPEMBAGUI_SETUPPROPERTIESWIDGET_H
