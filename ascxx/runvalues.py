"""Literal CLI value assignments using ASCEND's instance and units APIs.

This is not an ASCEND or Python expression evaluator. Assignments leave
FIX/FREE state alone; structural constants must be set by model refinement.
"""

import math
import re


_IDENTIFIER = r"[A-Za-z_][A-Za-z_0-9]*"
_INDEX = r"\[\s*(?:[+-]?[0-9]+|'[^'\r\n]*')\s*\]"
_MEMBER = _IDENTIFIER + "(?:" + _INDEX + ")*"
_PATH = _MEMBER + r"(?:\." + _MEMBER + ")*"
_ASSIGNMENT = re.compile(r"\s*(" + _PATH + r")\s*=\s*(.*?)\s*", re.DOTALL)
_TOKEN = re.compile(r"(" + _IDENTIFIER + r")|\[\s*([+-]?[0-9]+|'[^'\r\n]*')\s*\]")
_REAL = re.compile(r"([+-]?(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?)"
                   r"\s*(?:\{([^{}]+)\})?")


def resolve_instance(sim, path):
    """Resolve a dotted path with integer/symbol array indices, without eval."""
    import ascpy

    if not isinstance(path, str) or not re.fullmatch(_PATH, path):
        raise ValueError(f"Invalid instance path {path!r}; expected names and scalar array indices")
    instance = sim.getModel()
    try:
        for token in _TOKEN.finditer(path):
            name, index = token.groups()
            if name is not None:
                # Call the native child lookup explicitly, so names colliding
                # with Python attributes cannot escape into Python objects.
                instance = ascpy.Instance.__getattr__(instance, name)
            else:
                if not instance.isArray():
                    raise ValueError("Indexed target is not an array")
                if index.startswith("'"):
                    instance = instance[index[1:-1]]
                else:
                    instance = instance[int(index)]
    except Exception as error:
        raise ValueError(f"Cannot resolve {path!r}: {error}") from error
    return instance


def apply_overrides(sim, assignments):
    """Apply PATH=LITERAL assignments in order; duplicate paths use the last value.

    Reals optionally carry {units}; bare numbers use base units. Booleans are
    TRUE/FALSE, integers use integer literals, symbols/selectors are quoted.
    Stops at the first error, without rolling back earlier assignments.
    """
    import ascpy

    if isinstance(assignments, str):
        assignments = [assignments]
    for assignment in assignments:
        match = _ASSIGNMENT.fullmatch(assignment)
        if match is None:
            raise ValueError(f"Invalid override {assignment!r}; expected PATH=VALUE, optionally with {{units}}")
        path, literal = match.groups()
        try:
            instance = resolve_instance(sim, path)
            if instance.isConst():
                raise ValueError("Cannot override a constant; use model parameters or a structural refinement")
            if instance.isArray() or instance.isModel():
                raise ValueError("Override target must be a scalar")
            if instance.isReal():
                value_match = _REAL.fullmatch(literal)
                if value_match is None:
                    raise ValueError("Expected a finite real literal, optionally followed by {units}")
                number, units = value_match.groups()
                value = float(number)
                if not math.isfinite(value):
                    raise ValueError("Real value must be finite")
                if units is not None:
                    units = units.strip()
                    if not units or units == "*":
                        raise ValueError("Expected explicit units inside braces")
                    conversion = ascpy.Units(units).getConversion()
                    if not math.isfinite(value * conversion):
                        raise ValueError("Converted value must be finite")
                    instance.setRealValueWithUnits(value, units)
                else:
                    instance.setRealValue(value)
            elif instance.isBool():
                if literal.upper() not in ("TRUE", "FALSE"):
                    raise ValueError("Expected TRUE or FALSE")
                instance.setBoolValue(literal.upper() == "TRUE")
            elif instance.isInt():
                if not re.fullmatch(r"[+-]?[0-9]+", literal):
                    raise ValueError("Expected an integer literal")
                instance.setIntValue(int(literal))
            elif instance.isSymbol() or instance.isSelector():
                if not re.fullmatch(r"'[^'\r\n]*'", literal):
                    raise ValueError("Expected a single-quoted symbol literal")
                setter = instance.setSelectorValue if instance.isSelector() else instance.setSymbolValue
                setter(literal[1:-1])
            else:
                raise ValueError("Override target must be a real, integer, boolean, symbol or selector")
        except Exception as error:
            raise ValueError(f"Override {assignment!r}: {error}") from error
