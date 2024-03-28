import mpemba
import datetime
import sys
from PyQt5.QtWidgets import *


if __name__ == '__main__':
    case = mpemba.readExperiment("TestData/case_file1.cf")
    exp_setup = case[0]
    time_recorded = case[1]
    video_file = case[2]
    exp = mpemba.Experiment(video_file, exp_setup, datetime.datetime.now())
    sim_parameters = mpemba.SimulationParameters(1E-4, 1E-1)
    sim = mpemba.ThermalSimulation(exp_setup, sim_parameters)
    sim.simulate()
    sim_results = sim.get_results()
    supercorrector = mpemba.SupercoolingCorrector(exp, sim_results, exp_setup)
    supercorrector.droplet_speed = 0.017535225760000002 # manually correct as no speed was measured here
    super_time = supercorrector.get_supercooling_time()
    super_error = supercorrector.get_supercooling_error()
    mpemba.printSci(super_time, "Supercooling time")
    mpemba.printSci(super_error, "Supercooling error")
    res_calculator = mpemba.ResultsCalculator(exp, super_time)
    volume = 1.41338e-10
    res_calculator.volume = volume
    nucleation_rate = res_calculator.get_nucleation_rate()
    mpemba.printSci(nucleation_rate, "Nucleation rate")
    error_calculator = mpemba.ErrorCalculator(exp, super_time, super_error, nucleation_rate)
    error_calculator.volume_error = 1.863407635991848e-12
    error_calculator.volume = volume
    error = error_calculator.get_error("nonlinear")
    mpemba.printSci(error[1], "Positive error")
    mpemba.printSci(error[0], "Negative error")

    mpemba.readExperiment("TestData/case_file1.cf")


    """app = QApplication(sys.argv)
    w = QWidget()
    w.resize(800, 600)
    w.setWindowTitle("Mpemba Data Analysis")
    table = QTableWidget()
    table.setRowCount(0)
    table.setColumnCount(5)
    column_names = ["Video file",
                    "Water flow rate",
                    "Oil flow rate",
                    "Initial temperature",
                    "Cooler temperature"]
    table.setHorizontalHeaderLabels(column_names)
    header = table.horizontalHeader()
    header.setSectionResizeMode(0, QHeaderView.Stretch)
    for i in range(1, len(column_names)):
        header.setSectionResizeMode(i, QHeaderView.ResizeToContents)

    splitter_line = QFrame()
    splitter_line.setFrameShape(QFrame.HLine)
    splitter_line.setFrameShadow(QFrame.Sunken)

    splitter_line2 = QFrame()
    splitter_line2.setFrameShape(QFrame.HLine)
    splitter_line2.setFrameShadow(QFrame.Sunken)

    fileedit = QLineEdit()
    fileedit.setReadOnly(True)

    browse_file_button = QPushButton("Browse")

    hfilebox = QHBoxLayout()
    hfilebox.addWidget(fileedit)
    hfilebox.addWidget(browse_file_button)

    ######################
    water_flow_box = QHBoxLayout()
    water_flow_label = QLabel("Water flow rate")
    water_flow_edit = QLineEdit()
    water_flow_unit_label = QLabel("µL  <span>&#183;</span>  min <sup>-1</sup>")
    water_flow_box.addWidget(water_flow_label)
    water_flow_box.addWidget(water_flow_edit)
    water_flow_box.addWidget(water_flow_unit_label)

    oil_flow_box = QHBoxLayout()
    oil_flow_label = QLabel("Oil flow rate")
    oil_flow_edit = QLineEdit()
    oil_flow_unit_label = QLabel("µL  <span>&#183;</span>  min <sup>-1</sup>")
    oil_flow_box.addWidget(oil_flow_label)
    oil_flow_box.addWidget(oil_flow_edit)
    oil_flow_box.addWidget(oil_flow_unit_label)

    initial_temp_box = QHBoxLayout()
    initial_temp_label = QLabel("Initial temperature")
    initial_temp_edit = QLineEdit()
    initial_temp_unit_label = QLabel("°C")
    initial_temp_box.addWidget(initial_temp_label)
    initial_temp_box.addWidget(initial_temp_edit)
    initial_temp_box.addWidget(initial_temp_unit_label)

    cooler_temp_box = QHBoxLayout()
    cooler_temp_label = QLabel("Cooler temperature")
    cooler_temp_edit = QLineEdit()
    cooler_temp_unit_label = QLabel("°C")
    cooler_temp_box.addWidget(cooler_temp_label)
    cooler_temp_box.addWidget(cooler_temp_edit)
    cooler_temp_box.addWidget(cooler_temp_unit_label)
    ######################

    vbox = QVBoxLayout(w)
    vbox.addLayout(water_flow_box)
    vbox.addLayout(oil_flow_box)
    vbox.addLayout(initial_temp_box)
    vbox.addLayout(cooler_temp_box)
    vbox.addLayout(hfilebox)
    vbox.addWidget(splitter_line)
    vbox.addWidget(table)
    vbox.addWidget(splitter_line2)
    w.show()
    sys.exit(app.exec_())"""