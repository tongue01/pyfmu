from pyfmu.fmi2slave import Fmi2Slave
from pyfmu.fmi2types import Fmi2Causality, Fmi2Variability, Fmi2DataTypes


class SineGenerator(Fmi2Slave):

    def __init__(self):

        author = ""
        modelName = "SineGenerator"
        description = ""

        super().__init__(
            modelName=modelName,
            author=author,
            description=description)

        self.register_variable("amplitude", data_type=Fmi2DataTypes.real,
                               variability=Fmi2Variability.fixed, causality=Fmi2Causality.parameter, start=1)
                               
        self.register_variable("frequency", data_type=Fmi2DataTypes.real,
                               variability=Fmi2Variability.fixed, causality=Fmi2Causality.parameter, start=1)

        self.register_variable(
            "phase", data_type=Fmi2DataTypes.real, causality=Fmi2Causality.parameter, variability = Fmi2Variability.fixed, start=0)

    def setup_experiment(self, start_time: float):
        pass

    def enter_initialization_mode(self):
        pass

    def exit_initialization_mode(self):
        pass

    def do_step(self, current_time: float, step_size: float) -> bool:
        return True

    def reset(self):
        pass

    def terminate(self):
        pass
