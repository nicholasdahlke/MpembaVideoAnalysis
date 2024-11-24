from fontTools.afmLib import error

import mpemba
import sys
import os
import toml
import matplotlib.pyplot as plt


if __name__ == '__main__':
    file_list = sys.argv
    file_list.pop(0)  # Remove the code from the argument list
    for file in file_list:
        if not os.path.isfile(file):
            raise Exception("One of the provided case files does not exist. " + file)

    initial_temperature = []
    nucleation_rate = []
    error_pos = []
    error_neg = []

    for file in file_list:
        with open(file, "r") as toml_file:
            toml_dict = toml.loads(toml_file.read())
            initial_temperature.append(toml_dict["data"]["initial_temperature"])
            nucleation_rate.append(toml_dict["results"]["nucleation_rate"])
            error_pos.append(toml_dict["results"]["error_pos"])
            error_neg.append(toml_dict["results"]["error_neg"])

    #plt.errorbar(initial_temperature, nucleation_rate, yerr=(error_neg, error_pos), linestyle="")
    plt.scatter(initial_temperature, nucleation_rate)
    plt.xlabel("$T_h$")
    plt.ylabel(r"$\dot{N}$")
    plt.show()