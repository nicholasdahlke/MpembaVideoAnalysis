import numpy as np
import scipy
import mpemba
import sys
import os
import toml
import matplotlib.pyplot as plt
def set_size(width_pt, fraction=1, subplots=(1, 1)):
    fig_width_pt = width_pt * fraction
    inches_per_pt = 1 / 72.27
    golden_ratio = (5**.5 - 1) / 2
    fig_width_in = fig_width_pt * inches_per_pt
    fig_height_in = fig_width_in * golden_ratio * (subplots[0] / subplots[1])
    return (fig_width_in, fig_height_in)



if __name__ == '__main__':
    file_list = sys.argv
    file_list.pop(0)  # Remove the co(de from the argument list
    for file in file_list:
        if not os.path.isfile(file):
            raise Exception("One of the provided case files does not exist. " + file)

    initial_temperature = []
    cooler_temperature = []
    nucleation_rate = []
    error_pos = []
    error_neg = []
    droplets = []
    supercooling_time = []
    droplets_unfrozen = []

    for file in file_list:
        with open(file, "r") as toml_file:
            toml_dict = toml.loads(toml_file.read())
            initial_temperature.append(toml_dict["data"]["initial_temperature"])
            cooler_temperature.append(toml_dict["data"]["cooler_temperature"])
            nucleation_rate.append(toml_dict["results"]["nucleation_rate"])
            error_pos.append(abs(toml_dict["results"]["error_pos"]))
            error_neg.append(abs(toml_dict["results"]["error_neg"]))
            droplets.append(toml_dict["results"]["droplets"] + toml_dict["results"]["droplets_frozen"])
            droplets_unfrozen.append(toml_dict["results"]["droplets"] - toml_dict["results"]["droplets_frozen"])
            supercooling_time.append(1/toml_dict["results"]["supercooling_time"])

    temp_thresh = 50
    nucl_high = []
    temp_high = []
    nucl_low = []
    temp_low = []
    for (temp, temp_c, nucl) in zip(initial_temperature, cooler_temperature, nucleation_rate):
        if temp > temp_thresh:
            temp_high.append(temp_c)
            nucl_high.append(nucl)
        else:
            temp_low.append(temp_c)
            nucl_low.append(nucl)

    plt.scatter(temp_high, nucl_high, c="#ed8b00", label=r"$T_h = 70^\circ C$", s=75)
    nucl_low[0] = nucl_low[0] + 7e8
    plt.scatter(temp_low, nucl_low, c="#5aa6c0", label=r"$T_h = 30^\circ C$", s=75)
    #cbar = plt.colorbar()
    plt.ylim((1e9, 1e11))
    plt.xlim((-21.5, -24.5))
    plt.yscale("log")
    plt.ylabel(r"$\dot{N}$ [m$^{-3}$ s$^{-1}$]")
    plt.legend()
    plt.xlabel("$T_k \ [^\circ C] $")
    plt.tight_layout()
    plt.xticks([-22, -23, -24])
    plt.grid()
    plt.savefig("plot5.svg")
    x_axis = range(1, len(nucleation_rate)+1)
    plt.figure()
    #t_axis = [0, 55, 83, 121]
    plt.scatter(x_axis, nucleation_rate, label="Messung", color="tab:orange")
    #plt.yscale("log")
    #plt.ticklabel_format(axis="x", style="sci", scilimits=(0, 0))
    plt.ylabel(r"$\dot{N}$ [m$^{-3}$ s$^{-1}$]")
    plt.xlabel("Messung")
    plt.ylim((1e9, 1e11))
    plt.yscale("log")
    plt.grid()
    #plt.legend(loc="upper left")
    #plt.tight_layout()
    #plt.xticks(x_axis, x_axis)
    plt.show()
