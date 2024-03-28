import time
import numpy as np
from sklearn.cluster import KMeans
import matplotlib.pyplot as plt

def read_points_from_file(file_path):
    pfns = []
    with open(file_path, 'r') as file:
        for line in file:
            _, pfn_field = line.strip().split(',')
            _, pfn_value = pfn_field.split('=')
            pfns.append([int(pfn_value)])
    return np.array(pfns)

file_path = 'pte_map'  
pfns = read_points_from_file(file_path)

print("Applying kmeans")
stime = time.time()

kmeans = KMeans(n_clusters=2, random_state=0)  
kmeans.fit(pfns)
centroids = kmeans.cluster_centers_
labels = kmeans.labels_

etime = time.time()
print("Done in:", etime - stime, "seconds")

# Plot clusters
colors = ['r', 'g', 'b', 'y', 'c', 'm']
for i in range(len(pfns)):
    plt.scatter(pfns[i][0], 0, color=colors[labels[i]], alpha=0.5)

# Plot centroids
plt.scatter(centroids[:, 0], np.zeros_like(centroids[:, 0]), marker='x', color='k', label='Centroids')

plt.xlabel('PFN')
plt.title('Clustering of PFN points')
plt.legend()
plt.show()

