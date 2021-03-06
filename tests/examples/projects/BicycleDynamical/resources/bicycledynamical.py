from pyfmu.fmi2slave import Fmi2Slave
from pyfmu.fmi2types import Fmi2Causality, Fmi2Variability, Fmi2DataTypes

class BicycleDynamical(Fmi2Slave):

    def __init__(self):
        
        author = ""
        modelName = "BicycleDynamical"
        description = ""    
        
        super().__init__(
            modelName=modelName,
            author=author,
            description=description)

        """
        Inputs, outputs and parameters may be defined using the 'register_variable' function:

        self.register_variable("my_input", data_type=Fmi2DataTypes.real, causality = Fmi2Causality.input, start=0)
        self.register_variable("my_output", data_type=Fmi2DataTypes.real, causality = Fmi2Causality.output)
        self.register_variable("my_parameter", data_type=Fmi2DataTypes.real, causality = Fmi2Causality.parameter, start=0)
        """

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