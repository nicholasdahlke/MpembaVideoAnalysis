//
// Created by nicholas on 25.05.24.
//

#ifndef MPEMBAGUI_WINDOW_H
#define MPEMBAGUI_WINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QTableWidget>
#include <QTabWidget>
#include <QHeaderView>
#include <QLabel>
#include <QCheckBox>
#include <QFileDialog>
#include <QMetaType>
#include <QTextEdit>
#include <QPixmap>
#include <QFont>
#include <QBitmap>
#include <QtConcurrent/QtConcurrent>
#include <QThread>
#include <QProcess>
#include <QScrollBar>
#include <QMessageBox>
#include "SetupPropertiesWidget.h"
#include "RunPropertiesWidget.h"
#include "FilePropertiesWidget.h"
#include "Measurement.h"
#include <vector>
#include <memory>
#include <QFuture>

class Window : public QWidget
{
    Q_OBJECT
    public:
        explicit Window(QWidget *parent = nullptr);

    private:
        QGridLayout *window_layout;

        QLabel *ae_logo;
        QLabel *title;

        QPushButton *video_open_button;
        QPushButton *case_open_button;

        QTableWidget *file_table;

        QPushButton *update_button;

        QTabWidget *properties_tab;
        SetupPropertiesWidget *setupProperties;
        RunPropertiesWidget *runProperties;
        FilePropertiesWidget *fileProperties;

        QPushButton *generate_case_files_button;

        QPushButton *run_video_button;
        QPushButton *run_evaluation_button;

        QLabel *status_label;

        std::vector<Measurement> measurements;
        std::vector<QString> videos_processed;


        QFutureWatcher<void> watcher;

        void execute_video_analysis();
        void execute_data_evaluation();

    signals:
    private slots:
        void video_open_button_clicked();
        void update_button_clicked();
        void table_item_double_clicked(int row, int);
        void generate_case_files();
        void run_video_clicked();
        void run_evaluation_clicked();
        void analysis_finished();


};


#endif //MPEMBAGUI_WINDOW_H
