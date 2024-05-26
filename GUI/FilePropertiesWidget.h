//
// Created by nicholas on 25.05.24.
//

#ifndef MPEMBAGUI_FILEPROPERTIESWIDGET_H
#define MPEMBAGUI_FILEPROPERTIESWIDGET_H
#include "Measurement.h"
#include <QFormLayout>
#include <QLabel>
#include <QCheckBox>

class FilePropertiesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FilePropertiesWidget(QWidget *parent = 0);

    QFormLayout *layout;

    QLabel *filename_label;
    QLabel *case_label;
    QCheckBox *flip;

    void setFromMeasurement(const Measurement& measurement) const;
    void writeToMeasurement(Measurement& measurement) const;

    signals:
public slots:

};

#endif //MPEMBAGUI_FILEPROPERTIESWIDGET_H
