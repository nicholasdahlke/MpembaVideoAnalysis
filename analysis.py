import numpy as np
import matplotlib.pyplot as plt
from scipy import stats

filename = "/mnt/md0/Progammiersoftwareprojekte/CLionProjects/MpembaVideoAnalysis/videos/volumes.csv"
data = np.genfromtxt(filename, delimiter=';')

volumes = data[:,1]
volumes = volumes#[volumes<0.5e6]
volume = stats.mode(volumes)
print("Droplet volume is" + str(volume))
plt.hist(volumes, bins=1000)
#plt.vlines(volume, 0, plt.ylim())
plt.show()