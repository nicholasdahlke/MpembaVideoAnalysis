import mpemba
import sys
import os

if __name__ == '__main__':
    file_list = sys.argv
    file_list.pop(0) # Remove the code from the argument list
    for file in file_list:
        if not os.path.isfile(file):
            raise Exception("One of the provided case files does not exist.")

    for file in file_list:
        case = mpemba.readExperiment(file)
        exp_setup = case[0]
        time_recorded = case[1]
        video_file = case[2]
        mpemba.print_(exp_setup.initial_temperature, "Initial temperature")
        mpemba.print_(exp_setup.cooler_temperature, "Cooler temperature")
        exp = mpemba.Experiment(video_file, exp_setup, time_recorded)
        mpemba.print_(exp.get_file_content("droplet_count"), "Unfrozen droplets")
        mpemba.print_(exp.get_file_content("droplet_count_frozen"), "Frozen droplets")
        mpemba.printSci(exp.get_file_content("droplet_volumes"), "Droplet volume")
        sim_parameters = mpemba.SimulationParameters(1E-4, 1E-1)
        sim = mpemba.ThermalSimulation(exp_setup, sim_parameters)
        sim.simulate()
        sim_results = sim.get_results()
        supercorrector = mpemba.SupercoolingCorrector(exp, sim_results, exp_setup)
        super_time = supercorrector.get_supercooling_time()
        super_error = supercorrector.get_supercooling_error()
        res_calculator = mpemba.ResultsCalculator(exp, super_time)
        nucleation_rate = res_calculator.get_nucleation_rate()
        mpemba.printSci(nucleation_rate, "Nucleation rate")
        error_calculator = mpemba.ErrorCalculator(exp, super_time, super_error, nucleation_rate)
        error = error_calculator.get_error("nonlinear")
        mpemba.printSci(error[1], "Positive error")
        mpemba.printSci(error[0], "Negative error")
        print("-"*20)
