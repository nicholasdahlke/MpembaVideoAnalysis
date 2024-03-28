import datetime
import os
import csv
import numpy as np
from dataclasses import dataclass
import scipy
import tomllib


@dataclass
class ExperimentalSetup:
    flow_rate_oil_ul: float
    flow_rate_water_ul: float
    frames_per_second: float
    initial_temperature: float
    cooler_temperature: float
    cooler_length: float
    thermal_conductivity_tubing: float
    inner_radius_tubing: float
    outer_radius_tubing: float
    water_density: float


@dataclass
class SimulationParameters:
    time_step: float
    convergence_threshold: float  # Converge if result differs this much from last one


@dataclass
class SimulationResults:
    time_steps: np.array
    results: np.array


def errorMessage(message: str):
    print("ERROR: " + message)


def checkIfNone(variables: list):
    for variable in variables:
        if variable is None:
            errorMessage("Value missing, input manually")
            return True
    return False


def printSci(value, name: str, precision: int = 3):
    print(name, "{0:.{1:d}e}".format(value, precision))


def readExperiment(filename: str):
    with open(filename, "rb") as toml:
        toml_dict = tomllib.load(toml)
    video_filename = toml_dict["data"]["video"]
    flow_rate_oil_ul = float(toml_dict["data"]["flow_rate_oil_ul"])
    flow_rate_water_ul = float(toml_dict["data"]["flow_rate_water_ul"])
    frames_per_second = float(toml_dict["data"]["frames_per_second"])
    initial_temperature = float(toml_dict["data"]["initial_temperature"])
    cooler_temperature = float(toml_dict["data"]["cooler_temperature"])
    cooler_length = float(toml_dict["data"]["cooler_length"])
    thermal_conductivity_tubing = float(toml_dict["data"]["thermal_conductivity_tubing"])
    inner_radius_tubing = float(toml_dict["data"]["inner_radius_tubing"])
    outer_radius_tubing = float(toml_dict["data"]["outer_radius_tubing"])
    water_density = float(toml_dict["data"]["water_density"])
    data_recorded = datetime.datetime.strptime(toml_dict["data"]["date_recorded"], '%Y-%m-%d %H:%M:%S')
    r_exp_setup = ExperimentalSetup(flow_rate_oil_ul=flow_rate_oil_ul,
                                    flow_rate_water_ul=flow_rate_water_ul,
                                    frames_per_second=frames_per_second,
                                    initial_temperature=initial_temperature,
                                    cooler_temperature=cooler_temperature,
                                    cooler_length=cooler_length,
                                    thermal_conductivity_tubing=thermal_conductivity_tubing,
                                    inner_radius_tubing=inner_radius_tubing,
                                    outer_radius_tubing=outer_radius_tubing,
                                    water_density=water_density)
    return r_exp_setup, data_recorded, video_filename



class Experiment:
    def __init__(self, video_filename: str, setup: ExperimentalSetup, time_executed: datetime.datetime):

        self.setup = setup
        self.time_executed = time_executed
        self.video_filename = video_filename
        self.filedict = {
            "droplet_volume": "",
            "droplet_count": "",
            "droplet_distance": "",
            "droplet_speed": "",
            "droplet_count_frozen": "",
            "droplet_calibration": ""
        }

        self._contentdict = {}

        for file_key in self.filedict:
            self._contentdict[file_key] = np.array([])

        self._set_filenames()
        self._read_files()

    def _set_filenames(self):
        self._set_filename_check_if_exists("droplet_volumes", "-volumes.csv")
        self._set_filename_check_if_exists("droplet_count", "-droplet_count.csv")
        self._set_filename_check_if_exists("droplet_distance", "-distances.csv")
        self._set_filename_check_if_exists("droplet_speed", "-speeds.csv")
        self._set_filename_check_if_exists("droplet_count_frozen", "-droplet_count_frozen.csv")
        self._set_filename_check_if_exists("droplet_calibration", "-calibration.csv")

    def _set_filename_check_if_exists(self, file, addition):
        vid_file_without_extension = self.video_filename.rsplit(".", 1)[0]
        self.filedict[file] = vid_file_without_extension + addition
        if not os.path.isfile(self.filedict[file]):
            self.filedict[file] = ""

    def _read_files(self):
        for file_key in self.filedict:
            if self.filedict[file_key] != "":
                file_content = []
                with open(self.filedict[file_key], newline='') as csvfile:
                    reader = csv.reader(csvfile, delimiter=';')
                    for row in reader:
                        file_content.append(float(row[1]))
                if len(file_content) == 0:
                    self.filedict[file_key] = ""
                else:
                    self._contentdict[file_key] = np.array(file_content)

    def get_file_content(self, file_key):
        if self.filedict[file_key] == "":
            return None

        match file_key:
            case "droplet_volume":
                return scipy.stats.mode(self._contentdict["droplet_volume"])[0]

            case "droplet_count":
                return self._contentdict["droplet_count"][0]

            case "droplet_distance":
                return scipy.stats.mode(self._contentdict["droplet_distance"])[0]

            case "droplet_speed":
                return scipy.stats.mode(self._contentdict["droplet_speed"])[0]

            case "droplet_count_frozen":
                return self._contentdict["droplet_count_frozen"][0]

            case "droplet_calibration":
                return self._contentdict["droplet_calibration"][0]

            case _:
                return None

    def get_file_content_array(self, file_key):
        if self.filedict[file_key] == "":
            return None
        return self._contentdict[file_key]


class ThermalSimulation:
    def __init__(self, experimental_setup: ExperimentalSetup, simulation_parameters: SimulationParameters):
        self._experimental_setup = experimental_setup
        self._simulation_parameters = simulation_parameters
        self._simulation_results = []
        self._time_steps = []
        self._oil_specific_heat_capacity = lambda t: 1.4982 * (t - 273.15) + 1091
        self._water_specific_heat_capacity = lambda t: (37.688 + 0.085778 * t + 5.3764E5 / t ** 2 + 3.663 * np.power(
            (t / 185 - 1), -0.99)) / 0.01801258
        self._total_heat_capacity = lambda t: (2 / 3) * self._water_specific_heat_capacity(t) + (
                1 / 3) * self._oil_specific_heat_capacity(t)

    def _dgl(self, _temperature, _thermal_conductivity_tubing, _inner_radius_tubing, _outer_radius_tubing,
             _fluid_density, _fluid_specific_heat_capacity, _cooler_temp):
        return ((2 * np.pi * _thermal_conductivity_tubing * (_temperature - _cooler_temp)) / (
                _fluid_density * np.pi * np.power(_inner_radius_tubing, 2) * _fluid_specific_heat_capacity)) / np.log(
            _inner_radius_tubing / _outer_radius_tubing)

    def simulate(self):
        current_time = 0
        iterations = 0
        self._simulation_results.append(self._experimental_setup.initial_temperature)
        self._time_steps.append(current_time)
        while np.abs(self._simulation_results[
                         -1] - self._experimental_setup.cooler_temperature) > self._simulation_parameters.convergence_threshold:
            current_time += self._simulation_parameters.time_step
            res = self._simulation_results[-1] + self._simulation_parameters.time_step * self._dgl(
                self._simulation_results[-1],
                self._experimental_setup.thermal_conductivity_tubing,
                self._experimental_setup.inner_radius_tubing,
                self._experimental_setup.outer_radius_tubing,
                self._experimental_setup.water_density,
                self._total_heat_capacity(self._simulation_results[-1] + 273.15),
                self._experimental_setup.cooler_temperature)
            self._simulation_results.append(res)
            self._time_steps.append(current_time)
            iterations += 1
        print("Finished simulation in {} iterations".format(iterations))

    def get_results(self):
        return SimulationResults(np.array(self._time_steps), np.array(self._simulation_results))


class ResultsCalculator:
    def __init__(self, experiment: Experiment, supercooling_time: float):
        self._experiment = experiment
        self._supercooling_time = supercooling_time
        self.volume = self._experiment.get_file_content("droplet_volume")
        self.time = self._supercooling_time
        self.unfrozen_count = self._experiment.get_file_content("droplet_count")
        self.total_count = self._experiment.get_file_content("droplet_count") + self._experiment.get_file_content(
            "droplet_count_frozen")

    def get_nucleation_rate(self):
        if checkIfNone([self.time, self.volume, self.unfrozen_count, self.total_count]):
            return None
        return -1 * (1 / (self.time * self.volume)) * np.log(self.unfrozen_count / self.total_count)


class SupercoolingCorrector:
    def __init__(self, experiment: Experiment, simulation_results: SimulationResults,
                 experimental_setup: ExperimentalSetup):
        self._experiment = experiment
        self._simulation_results = simulation_results
        self._experimental_setup = experimental_setup
        self.droplet_speed = self._experiment.get_file_content("droplet_speed")

    def _get_cutoff_time(self):
        cutoff = -32
        return self._simulation_results.time_steps[np.abs(self._simulation_results.results - cutoff).argmin()]

    def _get_cooler_exit_time(self):
        return self._simulation_results.time_steps[np.abs(self._simulation_results.time_steps - (
                self._experimental_setup.cooler_length / self.droplet_speed)).argmin()]

    def _get_temp_reached_time(self):
        diff = 0.5
        return self._simulation_results.time_steps[
            np.argmax(self._simulation_results.results < self._experimental_setup.cooler_temperature + diff)]

    def get_supercooling_time(self):
        return self._get_cooler_exit_time() - self._get_cutoff_time()

    def get_supercooling_error(self):
        return self._get_cutoff_time() - self._get_temp_reached_time()


class ErrorCalculator:
    def __init__(self, experiment: Experiment, supercooling_time: float, supercooling_error: float,
                 nucleation_rate: float):
        self._experiment = experiment
        self._supercooling_error = supercooling_error
        self._supercooling_time = supercooling_time
        self.volume = self._experiment.get_file_content("droplet_volume")
        self.time = self._supercooling_time
        self.unfrozen_count = self._experiment.get_file_content("droplet_count")
        self.total_count = self._experiment.get_file_content("droplet_count") + self._experiment.get_file_content(
            "droplet_count_frozen")
        self.time_error = self._supercooling_error
        self.unfrozen_error = -2
        self.total_error = -2
        self.volume_error = 0
        self.nucleation_rate = nucleation_rate
        checkIfNone([self.volume,
                     self.time,
                     self.unfrozen_count,
                     self.total_count,
                     self.volume_error,
                     self.time_error,
                     self.unfrozen_error,
                     self.total_error,
                     self.nucleation_rate])
        self._err_pos = 0
        self._err_neg = 0

    def get_error(self, method="linear"):
        if self.volume_error == 0:
            self.volume_error = np.std(self._experiment.get_file_content("droplet_volume"))
        match method:
            case "linear":
                self._linear()
            case "nonlinear":
                self._nonlinear()
            case _:
                errorMessage("Unrecognized method")

        return self._error_neg, self._error_pos

    def _linear(self):
        def partial_t(t, nu, n0, v):
            return np.log(nu / n0) / (v * t ** 2)

        def partial_v(v, nu, n0, t):
            return np.log(nu / n0) / (v ** 2 * t)

        def partial_nu(v, t, nu):
            return -1 / (v * t * nu)

        def partial_n0(v, t, n0):
            return 1 / (v * t * n0)

        checkIfNone([self.volume, self.unfrozen_count, self.total_count, self.volume_error, self.total_error,
                     self.nucleation_rate, self.time_error, self.unfrozen_error, self.time])

        n_volume_error = np.abs(
            partial_v(self.volume, self.unfrozen_count, self.total_count, self.time) * self.volume_error)
        n_time_error = partial_t(self.time, self.unfrozen_count, self.total_count, self.volume) * self.time_error
        n_unfrozen_count_error = partial_nu(self.volume, self.time, self.unfrozen_count) * self.unfrozen_error
        n_total_count_error = partial_n0(self.volume, self.time, self.total_count) * self.total_error

        errors_sym = np.array([n_time_error, n_unfrozen_count_error, n_total_count_error])
        errors_asym = np.array([n_volume_error])
        self._error_pos = np.sum(errors_asym) + np.sum(errors_sym[errors_sym > 0])
        self._error_neg = np.sum(errors_asym) + np.abs(np.sum(errors_sym[errors_sym < 0]))

    def _nonlinear(self):
        def nucleation_rate(t, nu, n0, v):
            return -1 * (1 / (t * v)) * np.log(nu / n0)

        self._error_pos = nucleation_rate(self.time + self.time_error, self.unfrozen_count + self.unfrozen_error,
                                          self.total_count, self.volume - self.volume_error) - self.nucleation_rate
        self._error_neg = self.nucleation_rate - nucleation_rate(self.time, self.unfrozen_count,
                                                                 self.total_count + self.total_error,
                                                                 self.volume + self.volume_error)
