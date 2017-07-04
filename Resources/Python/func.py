from collections import Sequence
import numpy as np

def ClipValue(input, min, max):
    if input > max:
        return max
    elif input < min:
        return min
    else:
        return input

def LeadingEdgeFlaps(mach, aoa):
    if mach < 0.76 and aoa > 0.0:
        return aoa * 4.0 / 3.0
    else:
        return 0.0

def tefKias(kias):
    return (250.0 - kias) * 9.0 / 20.0

def tefAoA(aoa):
    if aoa > 0.0 and aoa < 11.0:
        return aoa * 1.5
    elif aoa >= 11.0 and aoa <= 16.0:
        return 16.5
    elif aoa > 16.0 and aoa <= 25.0:
        return (25.0 - aoa) * 11.0 / 6.0
    else:
        return 0.0

def TrailingEdgeFlaps(mach, aoa):
    if isinstance(aoa, (Sequence, np.ndarray)):
        return [TrailingEdgeFlaps(mach, x) for x in aoa]
    if mach < 0.9 and aoa > 0.0:
        return tefAoA(aoa)
    elif mach >= 0.9 and mach < 1.05 and aoa > 0.0:
        return ClipValue(tefAoA(aoa), 0.0, 8.0)
    else:
        return 0.0

def ElevatorAoA(val, offset = 0.0):
    # quintic fit -100,20,0,5.8,100,-3
    # 5.8 - 0.115 x + 0.00027 x^2
    return (5.8 + offset) - (0.115 * val) + (0.00027 * val * val)

def ElevatorPitchRate(val, offset = 0.0):
    # quintic fit -100,25,0,0,100,-22.5
    # - 0.2375 x + 0.000125 x^2
    return offset - (0.2375 * val) + (0.000125 * val * val)

def ElevatorGForce(val, limitG = False, offset = 0.0):
    # quintic fit -100,10,0,1,100,-5
    # 1 - 0.075 x + 0.00015 x^2
    result = (1.0 + offset) - (0.075 * val) + (0.00015 * val * val)
    return 5.5 if (limitG and result > 5.5) else result

def AileronRollRate(val, offset = 0.0):
    # quintic fit -100,-240,-50,-45,0,0,50,45,100,240
    # 0.4 x + 0.0002 x^3
    return (0.4 * val) + (0.0002 * val * val * val) + offset

def HighAoA(val, offset = 0.0):
    # quintic fit -100,55,0,22,100,-10
    # 22. - 0.325 x + 0.00005 x^2
    return (22 + offset) - (0.325 * val) + (0.00005 * val * val)