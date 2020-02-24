import subprocess
from enum import Enum
import platform
import logging
from tempfile import mkdtemp
from os.path import realpath, join
from pathlib import Path

from pybuilder.resources.resources import Resources



class FMI_Versions(Enum):
    FMI2 = "fmi2"
    FMI3 = "fmi3"

class ValidationResult():
    """
    Represents the result of validating an FMU with different validation techniques.
    """

    def __init__(self):
        self.validation_tools = {}

    def set_result_for(self, tool : str, valid : bool, message = ""):
        self.validation_tools[tool] = {"valid" : valid, "message" : message}

    def get_result_for(self, tool : str):
        return self.validation_tools[tool]

    @property
    def valid(self):
        ss = {s['valid'] for s in self.validation_tools.values()}
        return False not in ss
            

def _has_fmpy() -> bool:
    try:
        import fmpy
    except:
        return False
    
    return True

def validate(fmu_archive: str, use_fmpy: bool = True, use_fmucheck: bool = False, use_vdmcheck: bool = False):

    if(use_fmpy and not _has_fmpy()):
        raise ImportError("Cannot validate exported module using fmpy, the module could not be loaded. Ensure that the package is installed.")

    valid = True

    if(use_fmpy):
        from fmpy import dump, simulate_fmu

        dump(fmu_archive)

        try:
            res = simulate_fmu(fmu_archive)
        except Exception as e:
            valid = False
            return repr(e)

    return None

def validate_project(project) -> bool:
    """Validate a project to ensure that it is consistent.
    
    Arguments:
        project {PyfmuProject} -- The project that is validated
    
    Returns:
        bool -- [description]
    """
    # TODO add validation
    return True


def validate_modelDescription(modelDescription : str, use_fmucheck = False, use_vdmcheck = False, vdmcheck_version = FMI_Versions.FMI2) -> ValidationResult :
    
    if(True not in {use_fmucheck, use_vdmcheck}):
        raise ValueError("arguments must specifiy at least one verification tool")

    results = ValidationResult()

    if(use_vdmcheck):
        _validate_vdmcheck(modelDescription, results, vdmcheck_version)


    if(use_fmucheck):
        pass


    return results
    
def _vdmcheck_no_errors(results):
    """ Returns true if VDMCheck finds has found no errors.
    """
    return results.stdout == b'No errors found.\n'

def _validate_vdmcheck(modelDescription : str, validation_results : ValidationResult, fmi_version = FMI_Versions.FMI2):
    """Validate the model description using the VDMCheck tool.
    
    Arguments:
        modelDescription {str} -- textual representation of the model description.
    
    Keyword Arguments:
        fmi_version {FMI_Versions} -- [description] (default: {FMI_Versions.FMI2})
    
    Raises:
        ValueError: Raised if an the fmi_version is unknown or if the tool does not support validation thereof.
    """

    p = platform.system()

    bash_systems = {
        'Linux',
        'Solaris',
        'Darwin',
        "" # system could not be determined
    }

    powershell_systems = {
        'Windows'
    }

    script_path = None

    if (fmi_version == FMI_Versions.FMI2):
        
        if(p in bash_systems):
            script_path = Resources.get().vdmcheck_fmi2_sh

        elif(p in powershell_systems):
            script_path = Resources.get().vdmcheck_fmi3_ps

        else:
            raise ValueError(f'VDM checker for FMI 2 is not available for the current platform: {p}')

    elif (fmi_version == FMI_Versions.FMI3):
        if(p in bash_systems):
            script_path = Resources.get().vdmcheck_fmi3_sh

        elif(p in powershell_systems):
            script_path = Resources.get().vdmcheck_fmi3_ps

        else:
            raise ValueError(f'VDM checker for FMI 3 is not available for the current platform: {p}')

    else:
        raise ValueError(f'Unsupported FMI standard type, the standard {fmi_version} is not recognized.')



    # At the time of writing the checker can only be invoked on a file and not directly on a string.
    # Until this is introduced we simply store this in temp file

    tmpdir = Path(mkdtemp())
    md_path = tmpdir / 'modelDescription.xml'

    try:
        with open(md_path,'w') as f:
            f.write(modelDescription)

        result = subprocess.run([script_path,md_path],capture_output=True)

    finally:
        # TODO delete tmp dir
        pass
   
    isValid = _vdmcheck_no_errors(result)

    validation_results.set_result_for('vdmcheck',isValid, result.stdout)
        
    

    
    

    