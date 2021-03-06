
from collections import defaultdict
from .fmi2types import Fmi2Variability, Fmi2Causality, Fmi2Initial
from typing import Union, Any


# dictionaries defining valid combinations of variability and causality, see fmi2 p.49
_vc_combinations = defaultdict(dict)


_a_error = """The combinations “constant / parameter”, “constant / calculatedParameter” and “constant /
input” do not make sense, since parameters and inputs are set from the environment,
whereas a constant has always a value.
"""

_b_error = """The combinations “discrete / parameter”, “discrete / calculatedParameter”, “continuous /
parameter” and continuous / calculatedParameter do not make sense, since causality =
“parameter” and “calculatedParameter” define variables that do not depend on time, whereas
“discrete” and “continuous” define variables where the values can change during simulation.
"""

_c_error = "For an “independent” variable only variability = “continuous” makes sense."

_d_error = """A fixed or tunable “input” has exactly the same properties as a fixed or tunable parameter.
For simplicity, only fixed and tunable parameters shall be defined.
"""

_e_error = """A fixed or tunable “output” has exactly the same properties as a fixed or tunable
calculatedParameter. For simplicity, only fixed and tunable calculatedParameters shall be
defined.
"""

_A_initial = {"default": Fmi2Initial.exact, "possible": {Fmi2Initial.exact}}
_B_initial = {"default": Fmi2Initial.calculated,
              "possible": {Fmi2Initial.approx, Fmi2Initial.calculated}}
_C_initial = {"default": Fmi2Initial.calculated, "possible": {
    Fmi2Initial.exact, Fmi2Initial.approx, Fmi2Initial.calculated}}


_D_initial = {"default": None, "possible": {None}}
_E_initial = {"default": None, "possible": {None}}

_vc_combinations[Fmi2Variability.constant][Fmi2Causality.parameter] = {
    "err": _a_error}
_vc_combinations[Fmi2Variability.constant][Fmi2Causality.calculatedParameter] = {
    "err": _a_error}
_vc_combinations[Fmi2Variability.constant][Fmi2Causality.input] = {
    "err": _a_error}
_vc_combinations[Fmi2Variability.constant][Fmi2Causality.output] = {
    "err": None, "initial": _A_initial}
_vc_combinations[Fmi2Variability.constant][Fmi2Causality.local] = {
    "err": None, "initial": _A_initial}
_vc_combinations[Fmi2Variability.constant][Fmi2Causality.independent] = {
    "err": _c_error}


_vc_combinations[Fmi2Variability.fixed][Fmi2Causality.parameter] = {
    "err": None, "initial": _A_initial}
_vc_combinations[Fmi2Variability.fixed][Fmi2Causality.calculatedParameter] = {
    "err": None, "initial": _B_initial}
_vc_combinations[Fmi2Variability.fixed][Fmi2Causality.input] = {
    "err": _d_error}
_vc_combinations[Fmi2Variability.fixed][Fmi2Causality.output] = {
    "err": _e_error}
_vc_combinations[Fmi2Variability.fixed][Fmi2Causality.local] = {
    "err": None, "initial": _B_initial}
_vc_combinations[Fmi2Variability.fixed][Fmi2Causality.independent] = {
    "err": _c_error}

_vc_combinations[Fmi2Variability.tunable][Fmi2Causality.parameter] = {
    "err": None, "initial": _A_initial}
_vc_combinations[Fmi2Variability.tunable][Fmi2Causality.calculatedParameter] = {
    "err": None, "initial": _B_initial}
_vc_combinations[Fmi2Variability.tunable][Fmi2Causality.input] = {
    "err": _d_error}
_vc_combinations[Fmi2Variability.tunable][Fmi2Causality.output] = {
    "err": _e_error}
_vc_combinations[Fmi2Variability.tunable][Fmi2Causality.local] = {
    "err": None, "initial": _B_initial}
_vc_combinations[Fmi2Variability.tunable][Fmi2Causality.independent] = {
    "err": _c_error}


_vc_combinations[Fmi2Variability.discrete][Fmi2Causality.parameter] = {
    "err": _b_error}
_vc_combinations[Fmi2Variability.discrete][Fmi2Causality.calculatedParameter] = {
    "err": _b_error}
_vc_combinations[Fmi2Variability.discrete][Fmi2Causality.input] = {
    "err": None, "initial": _D_initial}
_vc_combinations[Fmi2Variability.discrete][Fmi2Causality.output] = {
    "err": None, "initial": _C_initial}
_vc_combinations[Fmi2Variability.discrete][Fmi2Causality.local] = {
    "err": None, "initial": _C_initial}
_vc_combinations[Fmi2Variability.discrete][Fmi2Causality.independent] = {
    "err": _c_error}

_vc_combinations[Fmi2Variability.continuous][Fmi2Causality.parameter] = {
    "err": _b_error}
_vc_combinations[Fmi2Variability.continuous][Fmi2Causality.calculatedParameter] = {
    "err": _b_error}
_vc_combinations[Fmi2Variability.continuous][Fmi2Causality.input] = {
    "err": None, "initial": _D_initial}
_vc_combinations[Fmi2Variability.continuous][Fmi2Causality.output] = {
    "err": None, "initial": _C_initial}
_vc_combinations[Fmi2Variability.continuous][Fmi2Causality.local] = {
    "err": None, "initial": _C_initial}
_vc_combinations[Fmi2Variability.continuous][Fmi2Causality.independent] = {
    "err": None, "initial": _D_initial}


def get_default_initial(variability: Fmi2Variability, causality: Fmi2Causality):

    if(validate_vc(variability, causality) is not None):
        raise Exception(
            f"Combinations of variability: {variability} and causality: {causality} is not allowed!")

    return _vc_combinations[variability][causality]["initial"]["default"]


def get_possible_initial(variability: Fmi2Variability, causality: Fmi2Causality):
    """ Returns the set of initial types that are valid for the combination of specific variability and causality.
    """
    if(validate_vc(variability, causality) is not None):
        raise Exception(
            f"Combinations of variability: {variability} and causality: {causality} is not allowed!")

    return _vc_combinations[variability][causality]["initial"]["possible"]


def should_define_start(variability: Fmi2Variability, causality: Fmi2Causality, initial: Fmi2Initial) -> bool:
    """Returns true if the combination requires that a start value is defined, otherwise false.

    For reference check the FMI2 specification p.54 for a description of which combination are allowed.
    """
    # see fmi2 spec p.54
    must_define_start = (initial in {Fmi2Initial.exact, Fmi2Initial.approx}
                         or causality in {Fmi2Causality.parameter, Fmi2Causality.input}
                         or variability in {Fmi2Variability.constant})

    can_not_define_start = (
        initial == Fmi2Initial.calculated or causality == Fmi2Causality.independent)

    # should be mutually exclusive
    assert(must_define_start != can_not_define_start)

    return must_define_start


def validate_start_value(variability: Fmi2Variability, causality: Fmi2Causality, initial: Fmi2Initial, start: Any) -> Union[str, None]:
    is_defined = start != None
    must_be_defined = should_define_start(variability, causality, initial)

    if(must_be_defined ^ is_defined):
        s = "must be defined" if not is_defined else "may not be defined"
        return f"Start values {s} for this combination of variability: {variability}, causality: {causality} and intial: {initial}"

    return None


def validate_vc(variability: Fmi2Variability, causality: Fmi2Causality):
    """Validate combinations of variablity and causality

    Arguments:
        variability {Fmi2Variability} -- [description]
        causality {Fmi2Causality} -- [description]

    Returns:
        [type] -- [description]
    """
    return _vc_combinations[variability][causality]["err"]
