import numpy as np
import matplotlib.pyplot as plt
from scipy import stats

filename = "/home/nicholas/Mpempa Videos/800Ul Oel 8Ul Wassel-volumes.csv"
data = np.genfromtxt(filename, delimiter=';')

volumes = data[:,1]
volumes = volumes * 1000 # Convert to liters
volumes = volumes * 1e6 # Convert to microliters
median_volume = np.median(volumes)
std_dev_volume = np.std(volumes)
print("Median is" + str(median_volume))
print("Standard Deviation:" + str(std_dev_volume))
plt.hist(volumes, bins=20)#range=[0, 0.1]
plt.figure()
plt.boxplot(volumes)
plt.show()