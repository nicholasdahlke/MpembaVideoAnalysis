import mpemba
import datetime

if __name__ == '__main__':

    file_list = ["/home/nicholas/Mpempa Videos/Ergebnisse neu/res1.cf",
                 "/home/nicholas/Mpempa Videos/Ergebnisse neu/res2.cf"]
    for file in file_list:
        case = mpemba.readExperiment(file)
        exp_setup = case[0]
        time_recorded = case[1]
        video_file = case[2]
        mpemba.print_(exp_setup.initial_temperature, "Initial temperature")
        mpemba.print_(exp_setup.cooler_temperature, "Cooler temperature")
        exp = mpemba.Experiment(video_file, exp_setup, datetime.datetime.now())

        mpemba.print_(exp.get_file_content("droplet_count"), "Unfrozen droplets")
        mpemba.print_(exp.get_file_content("droplet_count_frozen"), "Frozen droplets")
        mpemba.printSci(exp.get_file_content("droplet_volumes"), "Droplet volume")
        sim_parameters = mpemba.SimulationParameters(1E-4, 1E-1)
        sim = mpemba.ThermalSimulation(exp_setup, sim_parameters)
        sim.simulate()
        sim_results = sim.get_results()
        supercorrector = mpemba.SupercoolingCorrector(exp, sim_results, exp_setup)
        #supercorrector.droplet_speed = 0.017535225760000002 # manually correct as no speed was measured here
        super_time = supercorrector.get_supercooling_time()
        super_error = supercorrector.get_supercooling_error()
        res_calculator = mpemba.ResultsCalculator(exp, super_time)
        #volume = 1.41338e-10
        #res_calculator.volume = volume
        nucleation_rate = res_calculator.get_nucleation_rate()
        mpemba.printSci(nucleation_rate, "Nucleation rate")
        error_calculator = mpemba.ErrorCalculator(exp, super_time, super_error, nucleation_rate)
        #error_calculator.volume_error = 1.863407635991848e-12
        #error_calculator.volume = volume
        error = error_calculator.get_error("nonlinear")
        mpemba.printSci(error[1], "Positive error")
        mpemba.printSci(error[0], "Negative error")
        print("-"*20)
