import mpemba
import datetime

if __name__ == '__main__':
    video_file = "TestData/Frozen T Cool -33-0 T Warm 70-0 Oil 700 Water 10.mp4"
    exp_setup = mpemba.ExperimentalSetup(flow_rate_oil_ul=700,
                                         flow_rate_water_ul=10,
                                         frames_per_second=48.79,
                                         initial_temperature=70,
                                         cooler_temperature=-33,
                                         cooler_length=0.07,
                                         thermal_conductivity_tubing=.25,
                                         inner_radius_tubing=4.0e-4,
                                         outer_radius_tubing=7.9375e-4,
                                         water_density=997)
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
