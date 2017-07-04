import numpy as np
import matplotlib.pyplot as plt
import numpy.polynomial.polynomial as poly

def main():
    p = np.polyfit([-100,0,100], [10,1,-5], 3)
    f = np.poly1d(p)
    
    p2 = poly.polyfit([-100,0,100], [10,1,-5], 3)
    f2 = poly.Polynomial(p2)

    same = np.allclose(p, p2[::-1])
    print f
    pass


if __name__ == '__main__':
    main()