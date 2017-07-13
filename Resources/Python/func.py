from collections import Sequence
import numpy as np

def LeadingEdgeFlaps(mach, aoa):
    if mach < 0.76 and aoa > 0.0:
        return aoa * 4.0 / 3.0
    else:
        return 0.0

def tefKias(kias):
    return (250.0 - kias) * 9.0 / 20.0

def tefMax(mach):
    return 17.0 * np.clip(-0.28 + (4.62222222 * mach) - (4.14814815 * mach * mach), 0.0, 1.0)

def aoaFalloff(mach):
    return 15.7272727 * np.clip(1.14 - 0.23333333 * mach, 0.0, 1.0)

def tefAoA(aoa, mach):
    maxDeflection = tefMax(mach)
    slopeRising = 1.5
    slopeFalling = -11.0 / 6.0
    flatStart = maxDeflection / slopeRising
    flatEnd = aoaFalloff(mach)
    aoaEnd = (-maxDeflection / slopeFalling) + flatEnd
    if (aoa > 0.0 and aoa < flatStart):    
        return aoa * slopeRising    
    elif (aoa >= flatStart and aoa <= flatEnd):    
        return maxDeflection    
    elif (aoa > flatEnd and aoa <= aoaEnd):    
        return np.clip((slopeFalling * (aoa - flatEnd)) + maxDeflection, 0.0, maxDeflection)    
    else:    
        return 0.0

def TrailingEdgeFlaps(mach, aoa):
    if isinstance(aoa, (Sequence, np.ndarray)):
        return [TrailingEdgeFlaps(mach, x) for x in aoa]
    if (aoa > 0.0 and mach < 1.05):    
        return tefAoA(aoa, mach)    
    else:    
        return 0.0

def ElevatorAoA(val, offset = 0.0):
    # quintic fit {{-100,25},{0,5.8},{100,-10}}
    # 5.8 - 0.175 x + 0.00017 x^2
    return 5.8 + offset - (0.175 * val) + (0.00017 * val * val)

def ElevatorPitchRate(val, offset = 0.0):
    # '-1.1875e-05 x^3 + 0.000125 x^2 + -0.11875 x^1 + -3.07674029821e-15 x^0'
    return offset - (0.11875 * val) + (0.000125 * val * val) - (1.1875e-5 * val * val * val)

def ElevatorGForce(val, limitG = False, offset = 0.0):
    # '-3.75e-06 x^3 + 0.00015 x^2 + -0.0375 x^1 + 1.0 x^0'
    result = 1.0 + offset - (0.0375 * val) + (0.00015 * val * val) - ( 3.75e-6 * val * val * val )
    return 5.5 if (limitG and result > 5.5) else result

def AileronRollRate(val, offset = 0.0):
    # quintic fit -100,-240,-50,-45,0,0,50,45,100,240
    # 0.4 x + 0.0002 x^3
    return (0.4 * val) + (0.0002 * val * val * val) + offset

def HighAoA(val, offset = 0.0):
    # quintic fit -100,55,0,22,100,-10
    # 22. - 0.325 x + 0.00005 x^2
    return (22 + offset) - (0.325 * val) + (0.00005 * val * val)