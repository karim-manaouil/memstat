import re
import numpy as np

def read_file(filename):
    pfns = []
    with open(filename, 'r') as file:
        for line in file:
            match = re.search(r'pfn=(\d+)', line)
            if match:
                pfns.append(int(match.group(1)))
    return pfns

def calculate_distance(pfns):
    distances = [pfns[i+1] - pfns[i] for i in range(len(pfns)-1)]
    max_distance = np.max(distances)
    min_distance = np.min(distances)
    avg_distance = np.mean(distances)
    stddev_distance = np.std(distances)
    return avg_distance, stddev_distance, max_distance, min_distance

def main():
    filename = 'pte_map' 
    pfns = read_file(filename)
    avg_distance, stddev_distance, max_distance, min_distance = calculate_distance(pfns)
    print(f"max: {max_distance}")
    print(f"max: {min_distance}")
    print(f"Average distance between pfns: {avg_distance:.2f}")
    print(f"Standard deviation of distance between pfns: {stddev_distance:.2f}")

if __name__ == "__main__":
    main()

