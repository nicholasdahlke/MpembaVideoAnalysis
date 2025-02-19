import matplotlib.pyplot as plt
import numpy as np
import os
from scipy import optimize
from matplotlib import font_manager as fm
from scipy.optimize import curve_fit

fpath = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/PyDataEvaluation/Asap-Regular.ttf"
plt.rc("axes", axisbelow=True)
plt.rcParams.update({'font.size': 22})
plot_data = np.load("plot4.npy")
prop = fm.FontProperties(fname=fpath)
x_axis = [0, 19, 41, 61]
x_axis = np.array(x_axis)
#plt.scatter(plot_data[0,:], plot_data[1,:], color="#5aa6c0", s=75)
### Curve fit
fit = curve_fit(lambda x, a, b:a*np.exp(x*b), x_axis, plot_data[1,:])
print(fit)

##############

plt.plot(x_axis, fit[0][0]*np.exp(x_axis*fit[0][1]), c="tab:orange", zorder=10, label="Fit", lw=3)

plt.scatter(x_axis, plot_data[1,:], color="#5aa6c0", s=75, zorder=100, label="Messung")

plt.ylabel(r"$\dot{N}$ [m$^{-3}$ s$^{-1}$]", fontproperties=prop)
#plt.xlabel(r"$t \,[\text{min}]$", fontproperties=prop)
plt.xlabel(r"$t [\text{min}]$")
#plt.xticks([-22, -23, -24])
plt.ylim((1e8, 1e10))
#plt.xlim((-21.5, -24.5))
plt.yscale("log")
plt.grid()
plt.legend()
plt.tight_layout()
plt.savefig("plot4.svg")
plt.show()