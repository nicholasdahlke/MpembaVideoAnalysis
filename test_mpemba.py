from unittest import TestCase
import mpemba
import datetime


class TestMpemba(TestCase):
    def test_check_if_none(self):
        self.assertTrue(mpemba.checkIfNone([None, 1, 2, 3]))
        self.assertFalse(mpemba.checkIfNone([2, 1, 2, 3]))
        self.assertTrue(mpemba.checkIfNone([None, None, None, None]))


class TestCorrector(TestCase):
    def setUp(self):
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
        self._exp = mpemba.Experiment(video_file, exp_setup, datetime.datetime.min)
        sim_parameters = mpemba.SimulationParameters(1E-4, 1E-1)
        sim = mpemba.ThermalSimulation(exp_setup, sim_parameters)
        sim.simulate()
        sim_results = sim.get_results()
        self._supercorrector = mpemba.SupercoolingCorrector(self._exp, sim_results, exp_setup)
        self._supercorrector.droplet_speed = 0.017535225760000002

    def test_supercooling_time(self):
        self.assertEqual(self._supercorrector.get_supercooling_time(), 0.7174000000015139)

    def test_supercooling_error(self):
        self.assertEqual(self._supercorrector.get_supercooling_error(), -0.49920000000105347)


class TestCalculator(TestCorrector):
    def setUp(self):
        super(TestCalculator, self).setUp()
        self._res_calculator = mpemba.ResultsCalculator(self._exp, self._supercorrector.get_supercooling_time())
        self._res_calculator.volume = 1.41338e-10

    def test_nucleation_rate(self):
        self.assertEqual(self._res_calculator.get_nucleation_rate(), 19521089611.267918)


class TestErrorCalculator(TestCalculator):
    def setUp(self):
        super(TestErrorCalculator, self).setUp()
        self._error_calculator = mpemba.ErrorCalculator(self._exp, self._supercorrector.get_supercooling_time(),
                                                        self._supercorrector.get_supercooling_error(),
                                                        self._res_calculator.get_nucleation_rate())
        self._error_calculator.volume_error = 1.863407635991848e-12
        self._error_calculator.volume = 1.41338e-10

    def test_error_linear(self):
        self.assertEqual(self._error_calculator.get_error("linear"), (387134620.153697, 14780311222.173212))

    def test_error_nonlinear(self):
        self.assertEqual(self._error_calculator.get_error("nonlinear"), (382947122.5634575, 48806620878.419785))
