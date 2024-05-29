//
// Created by nicholas on 25.05.24.
//

#include "Window.h"
#include <iostream>

Window::Window(QWidget *parent) : QWidget(parent)
{
    // Set window parameters
    resize(700, 700);
    setWindowTitle("Mpemba GUI");

    // Create UI Objects
    window_layout = new QGridLayout();

    QPixmap ae_logo_pixmap("/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/GUI/ae_logo.png");
    ae_logo = new QLabel();
    ae_logo->setPixmap(ae_logo_pixmap.scaledToHeight(100));

    title = new QLabel("Mpæmba \n Data evaluation and video analysis");
    QFont title_font = title->font();
    title_font.setBold(true);
    title_font.setPointSize(20);
    title->setFont(title_font);

    video_open_button = new QPushButton("Open video");
    case_open_button = new QPushButton("Open case");
    case_open_button->setEnabled(false);

    update_button = new QPushButton("Update");

    file_table = new QTableWidget();

    properties_tab = new QTabWidget();
    setupProperties = new SetupPropertiesWidget();
    runProperties = new RunPropertiesWidget();
    fileProperties = new FilePropertiesWidget();

    run_video_button = new QPushButton("Run video analysis");
    run_video_button->setStyleSheet("color: rgb(0, 255, 0)");
    run_evaluation_button = new QPushButton("Run data evaluation");
    run_evaluation_button->setStyleSheet("color: rgb(0, 255, 0)");
    generate_case_files_button = new QPushButton("Generate case files");
    status_label = new QLabel("Idle");
    status_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);


    // Set table
    QStringList headers;

    headers.append("Filename");
    headers.append("Warm temperature");
    headers.append("Cold temperature");
    headers.append("Water flow rate");
    headers.append("Oil flow rate");

    file_table->setColumnCount(headers.size());
    file_table->setHorizontalHeaderLabels(headers);
    file_table->resizeColumnsToContents();
    QHeaderView *headerView = file_table->horizontalHeader();
    headerView->setSectionResizeMode(QHeaderView::Stretch);

    // Set tab widget
    properties_tab->addTab(fileProperties, "File properties");
    properties_tab->addTab(setupProperties, "Setup properties");
    properties_tab->addTab(runProperties, "Run properties");

    // Arrange layout
    window_layout->addWidget(title, 0, 0, 1, 1);
    window_layout->addWidget(ae_logo, 0, 2, 1, 1);
    window_layout->addWidget(video_open_button, 3, 0, 1, 1);
    window_layout->addWidget(case_open_button, 3, 1, 1, 1);
    window_layout->addWidget(file_table, 4, 0, 3, 3);
    window_layout->addWidget(update_button, 7, 0, 1, 3);
    window_layout->addWidget(properties_tab, 8, 0, 4, 3);
    window_layout->addWidget(generate_case_files_button, 12,0, 1, 1);
    window_layout->addWidget(run_video_button, 13, 0, 1, 1);
    window_layout->addWidget(run_evaluation_button, 13, 1, 1, 1);
    window_layout->addWidget(status_label, 13, 2, 1, 1);

    // Set layout
    setLayout(window_layout);

    // Connect signals
    connect(video_open_button, &QPushButton::released, this, &Window::video_open_button_clicked);
    connect(file_table, &QTableWidget::cellDoubleClicked, this, &Window::table_item_double_clicked);
    connect(update_button, &QPushButton::released, this, &Window::update_button_clicked);
    connect(generate_case_files_button, &QPushButton::released, this, &Window::generate_case_files);
    connect(run_video_button, &QPushButton::released, this, &Window::run_video_clicked);
    connect(&watcher, &QFutureWatcher<void>::finished, this, &Window::analysis_finished);
    connect(run_evaluation_button, &QPushButton::released, this, &Window::run_evaluation_clicked);

}

void Window::video_open_button_clicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Open video files", "~", "Video files (*.avi, *mp4)");
    for(const QString& file : files)
    {
        auto *filenameItem = new QTableWidgetItem(file);
        auto *waterflowItem = new QTableWidgetItem("0");
        auto *oilflowItem = new QTableWidgetItem("0");
        auto *initialItem = new QTableWidgetItem("0");
        auto *coolerItem = new QTableWidgetItem("0");

        filenameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        waterflowItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        oilflowItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        initialItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        coolerItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

        file_table->setRowCount(file_table->rowCount() + 1);
        file_table->setItem(file_table->rowCount() - 1, 0, filenameItem);
        file_table->setItem(file_table->rowCount() - 1, 1, initialItem);
        file_table->setItem(file_table->rowCount() - 1, 2, coolerItem);
        file_table->setItem(file_table->rowCount() - 1, 3, waterflowItem);
        file_table->setItem(file_table->rowCount() - 1, 4, oilflowItem);

        measurements.emplace_back(file);
    }
}

void Window::table_item_double_clicked(int row, int)
{
    QTableWidgetItem *clicked_item = file_table->item(row, 0);
    Measurement measurement = *std::find_if(measurements.begin(),
                 measurements.end(),
                 [file = clicked_item->text()]
                         (const Measurement & m) -> bool {return m.filename == file;}
                 );
    fileProperties->setFromMeasurement(measurement);
    setupProperties->setFromMeasurement(measurement);
    runProperties->setFromMeasurement(measurement);
}

void Window::update_button_clicked()
{
    QList<QTableWidgetItem*> selected = file_table->selectedItems();

    for (QTableWidgetItem* item : selected)
    {
        QString filename = file_table->item(item->row(), 0)->text();
        auto measurement = std::find_if(measurements.begin(),
                                                measurements.end(),
                                                [file = filename]
                                                        (const Measurement & m) -> bool {return m.filename == file;}
        );
        runProperties->writeToMeasurement(*measurement);
        setupProperties->writeToMeasurement(*measurement);
        fileProperties->writeToMeasurement(*measurement);
    }

    for (const Measurement& measurement : measurements)
    {
        QTableWidgetItem * item = file_table->findItems(measurement.filename, Qt::MatchExactly)[0];
        int row = item->row();
        file_table->item(row, 1)->setText(QString::number(measurement.initial_temperature));
        file_table->item(row, 2)->setText(QString::number(measurement.cooler_temperature));
        file_table->item(row, 3)->setText(QString::number(measurement.flow_rate_water_ul));
        file_table->item(row, 4)->setText(QString::number(measurement.flow_rate_oil_ul));
    }
}

void Window::generate_case_files()
{
    for (Measurement & measurement: measurements)
    {
        measurement.generateCaseFile();
    }
    run_evaluation_button->setEnabled(true);
}

void Window::run_video_clicked()
{
    status_label->setText("Running analysis");
    QFuture<void> video_future;
    video_future = QtConcurrent::run(this, &Window::execute_video_analysis);
    watcher.setFuture(video_future);
    run_video_button->setEnabled(false);
}

void Window::run_evaluation_clicked()
{
    status_label->setText("Running evaluation");
    QFuture<void> eval_future;
    eval_future = QtConcurrent::run(this, &Window::execute_data_evaluation);
    watcher.setFuture(eval_future);
    run_evaluation_button->setEnabled(false);
}

void Window::execute_video_analysis()
{

    QString flip_files = "";

    for (const Measurement & measurement : measurements)
    {
        if(measurement.flip_video == Qt::Checked)
            flip_files += "\"" +measurement.filename + "\" ";
    }
    std::cout << flip_files.toStdString();
    if(flip_files != "")
    {
        auto *run_video = new QProcess();
        QString executable = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/Scripts/flip.sh";
        QStringList arguments;
        arguments  << "-T" << "Mpemba" << "-e" << "bash" << "-c" << "exec " + executable + " " + flip_files;
        run_video->start("/usr/bin/xterm", arguments);
        run_video->waitForFinished(-1);
    }


    QString netfile = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/AI Models/multiphase.onnx";
    QString executable = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/Release/MpembaVideoAnalysis";
    auto measurement_it = measurements.begin();
    if(videos_processed.empty())
    {
        auto *run_video = new QProcess();
        run_video->start("/usr/bin/xterm", QStringList() << "-T" << "Mpemba" << "-e" << "bash" << "-c" << "exec " + executable + " \"" + measurement_it->filename + "\" \"" + netfile + "\"");
        run_video->waitForFinished(-1);
    }
    // Read calibration value
    std::filesystem::path video = measurement_it->filename.toStdString();
    std::filesystem::path cal = video.parent_path();
    cal += "/";
    cal += video.stem();
    cal += "-calibration.csv";
    std::ifstream cal_stream;
    cal_stream.open(cal, std::ios::in);
    std::string cal_file_content;
    cal_stream >> cal_file_content;
    cal_stream.close();
    std::string cal_value = cal_file_content.substr(cal_file_content.find((';')) + 1, cal_file_content.size());

    measurement_it++;
    while (measurement_it != measurements.end())
    {
        auto *run_video = new QProcess();
        run_video->start("/usr/bin/xterm", QStringList() << "-T" << "Mpemba" << "-e" << "bash" << "-c" << "exec " + executable + " \"" + measurement_it->filename + "\" \"" + netfile + "\" --calib-value=" + cal_value.c_str());
        run_video->waitForFinished(-1);
        measurement_it++;
    }
}

void Window::execute_data_evaluation()
{
    QString case_file_list;
    for (const Measurement & measurement:measurements)
    {
        if(measurement.case_file != "-")
            case_file_list += "\"" + measurement.case_file + "\" ";
    }
    if (!case_file_list.isEmpty())
    {
        QString executable = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/PyDataEvaluation/main.py";
        QStringList arguments;
        arguments  << "-hold" << "-T" << "Mpemba" << "-e" << "bash" << "-c" << "python3 " + executable + " " + case_file_list;
        auto *run_eval = new QProcess();
        run_eval->start("/usr/bin/xterm", arguments);
        run_eval->waitForFinished(-1);
    }
}

void Window::analysis_finished()
{
    run_video_button->setEnabled(true);
    run_evaluation_button->setEnabled(true);
    status_label->setText("Idle");
}