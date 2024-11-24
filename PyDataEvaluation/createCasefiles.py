import datetime
import toml
import sys
import os

if __name__ == "__main__":
    dir_list = sys.argv
    dir_list.pop(0)
    for directory in dir_list:
        if not os.path.isdir(directory):
            raise Exception("Directory " + directory + " does not exist")

    for directory in dir_list:
        for file in os.listdir(directory):
            if file.endswith(".mp4"):
                print(file)
                flow_rate_oil_ul = float(input("Oil flow rate:"))
                flow_rate_water_ul = float(input("Water flow rate:"))
                frames_per_second = float(input("Video FPS:"))
                initial_temperature = float(input("Initial temperature:"))
                cooler_temperature = float(input("Cooler temperature:"))
                date_recorded = str(datetime.date.today())
                if input("Use default setup parameters? (y/n)").strip().lower() != "y":
                    cooler_length = float(input("Cooler length:"))
                    thermal_conductivity = float(input("Thermal conductivity:"))
                    inner_radius_tubing = float(input("Inner radius tubing:"))
                    outer_radius_tubing = float(input("Outer radius tubing:"))
                    water_density = float(input("Water density:"))
                else:
                    cooler_length = 0.129115
                    thermal_conductivity = 0.25
                    inner_radius_tubing = 0.0004
                    outer_radius_tubing = 0.000794
                    water_density = 997.0
                case = {"data":{
                    "video":directory + "/" + file,
                    "flow_rate_oil_ul":flow_rate_oil_ul,
                    "flow_rate_water_ul":flow_rate_water_ul,
                    "frames_per_second":frames_per_second,
                    "initial_temperature":initial_temperature,
                    "cooler_temperature":cooler_temperature,
                    "cooler_length":cooler_length,
                    "thermal_conductivity_tubing":thermal_conductivity,
                    "inner_radius_tubing":inner_radius_tubing,
                    "outer_radius_tubing":outer_radius_tubing,
                    "water_density":water_density,
                    "date_recorded":date_recorded
                }}
                case_file = open(directory + "/" + file.split('.')[0]+".cf", "w")
                case_file.write(toml.dumps(case))
                case_file.close()
                print(40*"-")
