from pybuilder.resources.pyfmu.fmi2slave import Fmi2Slave
from pybuilder.resources.pyfmu.fmi2types import Fmi2DataTypes, Fmi2Causality, Fmi2Variability
from pybuilder.builder.modelDescription import extract_model_description_v2

class Adder(Fmi2Slave):
    
    def __init__(self):
        super().__init__("Adder")

        self.register_variable("a", data_type=Fmi2DataTypes.real, causality = Fmi2Causality.input, start=0)
        self.register_variable("b", data_type=Fmi2DataTypes.real, causality = Fmi2Causality.input, start=0)
        self.register_variable("c", data_type=Fmi2DataTypes.real,causality = Fmi2Causality.output)

        

class SineGenerator(Fmi2Slave):

    def __init__(self):
        super().__init__('SineGenerator')
        self.register_variable('amplitude',data_type = Fmi2DataTypes.real, causality= Fmi2Causality.parameter, start=1)
        self.register_variable('frequency', data_type = Fmi2DataTypes.real, causality=Fmi2Causality.parameter, start=1)
        self.register_variable('phase', data_type = Fmi2DataTypes.real, causality=Fmi2Causality.parameter, start=0)
        self.register_variable('y', data_type = Fmi2DataTypes.real, causality=Fmi2Causality.output)