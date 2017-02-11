import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
#from scipy.optimize import curve_fit
import numpy.polynomial.polynomial as poly

import func as fcs

# http://bkanuka.com/articles/native-latex-plots/
# https://combichem.blogspot.com/2013/11/my-best-fastest-and-awesomest-way-of.html

def figsize(width_pt, height_in = None):
    fig_width = width_pt * (1.0 / 72.27)
    fig_height = height_in if height_in is not None else fig_width * ((np.sqrt(5.0)-1.0) / 2.0)
    return (fig_width, fig_height)

#def fitfunc5(x, a, b, c, d, e, f):
#    return (a * (x ** 5)) + (b * (x ** 4)) + (c * (x ** 3)) + (d * (x ** 2)) + (e * x) + f

figsizeresult = figsize(469.75502 * 0.5)
figsizeresult1 = figsize(469.75502 * 0.9)
figsizeresult2 = figsize(469.75502 * 0.475)

figsize = (6,6)

#mpl.rc('font', size=10, family='serif', serif='cm')
#mpl.rc('legend', fontsize=10)
#mpl.rc('text', usetex=True)

mpl.rc('font', size=11)
mpl.rc('legend', fontsize=11)

pdf = PdfPages('fcs-plots.pdf')

def plotfunc(pdf, figsize, func, xaxis, yaxis, **kwargs):
    print('plotting {0} vs {1}'.format(xaxis, yaxis))

    #legendkw = {'fontsize':7, 'frameon':False} # etc - many other keywords

    fig = plt.figure(figsize=figsize)
    #ax = fig.add_axes([0.15, 0.15, 0.8, 0.8])
    ax = fig.add_axes([0.15, 0.1, 0.7, 0.7])

    ax.tick_params(width=0.2)

    ax.grid(True, which='both', linewidth=0.1)
    ax.axhline(y=0, color='k', linewidth=0.2)
    ax.axvline(x=0, color='k', linewidth=0.2)

    argx = np.linspace(-100.0, 100.0, num=2000)

    ax.plot(argx, func(argx), color='blue', linewidth=1)

    #ax.set_ylabel(yaxis, labelpad=0)
    #ax.set_xlabel(xaxis, labelpad=-1)
    ax.set_ylabel(yaxis, labelpad=5)
    ax.set_xlabel(xaxis, labelpad=5)
    ax.set_xlim(argx[0], argx[-1])

    if 'suptitle' in kwargs:
        fig.suptitle(kwargs['suptitle'])

    print('calculated size: ' + str(figsizeresult))
    print('figure size: ' + str(fig.get_size_inches()))
    plt.close(fig)
    pdf.savefig(fig)


def plottef(pdf, figsize, **kwargs):
    print('plotting trailing edge flaps')

    legendkw = {'fontsize':11, 'frameon':False} # etc - many other keywords
    #legendkw = {'fontsize':10, 'frameon':False} # etc - many other keywords

    fig = plt.figure(figsize=figsize)
    #ax = fig.add_axes([0.125, 0.15, 0.775, 0.8])
    ax = fig.add_axes([0.15, 0.1, 0.7, 0.7])

    ax.tick_params(width=0.2)

    ax.grid(True, which='both', linewidth=0.1)
    ax.axhline(y=0, color='k', linewidth=0.2)
    ax.axvline(x=0, color='k', linewidth=0.2)

    argx = np.linspace(0.0, 30.0, num=300)

    ax.plot(argx, fcs.TrailingEdgeFlaps(0.6, argx), color='blue', linewidth=2, label=r'$M<0.9$')
    ax.plot(argx, fcs.TrailingEdgeFlaps(0.9, argx), color='red', linewidth=1, label=r'$0.9\leq M<1.05$')

    ax.legend(loc=1, **legendkw)
    ax.set_ylabel('deflection [deg]', labelpad=10)
    ax.set_xlabel('angle of attack [deg]', labelpad=5)
    ax.set_xlim(argx[0], argx[-1])

    if 'suptitle' in kwargs:
        fig.suptitle(kwargs['suptitle'])

    print('calculated size: ' + str(figsizeresult))
    print('figure size: ' + str(fig.get_size_inches()))
    plt.close(fig)
    pdf.savefig(fig)

#plotfunc(pdf, figsizeresult2, fcs.ElevatorGForce, r'$\leftarrow$(stick position)$\rightarrow$', 'g-force')
#plotfunc(pdf, figsizeresult2, fcs.ElevatorPitchRate, r'$\leftarrow$(stick position)$\rightarrow$', 'pitch rate [deg/sec]')
#plotfunc(pdf, figsizeresult, fcs.ElevatorAoA, r'$\leftarrow$(stick position)$\rightarrow$', 'angle of attack [deg]')
#plotfunc(pdf, figsizeresult, fcs.HighAoA, r'$\leftarrow$(stick position)$\rightarrow$', 'angle of attack [deg]')
#plotfunc(pdf, figsizeresult, fcs.AileronRollRate, r'$\leftarrow$(stick position)$\rightarrow$', 'roll rate [deg]')
#plottef(pdf, figsizeresult1)

plotfunc(pdf, figsize, fcs.ElevatorGForce, r'$\leftarrow$(stick position)$\rightarrow$', 'g-force', suptitle='Up and away')
plotfunc(pdf, figsize, fcs.ElevatorPitchRate, r'$\leftarrow$(stick position)$\rightarrow$', 'pitch rate [deg/sec]', suptitle='Up and away')
plotfunc(pdf, figsize, fcs.ElevatorAoA, r'$\leftarrow$(stick position)$\rightarrow$', 'angle of attack [deg]', suptitle='Powered approach')
plotfunc(pdf, figsize, fcs.HighAoA, r'$\leftarrow$(stick position)$\rightarrow$', 'angle of attack [deg]', suptitle='Up and away (high AOA)')
plotfunc(pdf, figsize, fcs.AileronRollRate, r'$\leftarrow$(stick position)$\rightarrow$', 'roll rate [deg/sec]', suptitle='Roll rate')
plottef(pdf, figsize, suptitle='Trailing edge flaps')

pdf.close()