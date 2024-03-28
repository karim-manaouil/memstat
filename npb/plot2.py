import argparse
import matplotlib.pyplot as plt

def read_data(filename):
    data = {}
    with open(filename, 'r') as file:
        for line in file:
            parts = line.strip().split(',')
            app_name = parts[0]
            metrics = {item.split('=')[0]: float(item.split('=')[1]) for item in parts[1:]}
            data[app_name] = metrics
    return data

def calculate_ratio(data1, data2, metric):
    ratios = {}
    for app_name in data1:
        if app_name in data2:
            value1 = data1[app_name][metric]
            value2 = data2[app_name][metric]
            ratios[app_name] = value1 / value2
    return ratios

def plot_ratios(ratios):
    plt.figure(figsize=(10, 6))
    plt.barh(range(len(ratios)), list(ratios.values()), align='center')
    plt.yticks(range(len(ratios)), list(ratios.keys()))
    plt.xlabel('Ratio')
    plt.title('Ratio of metric for each application')
    plt.grid(axis='x')
    plt.show()

def main():
    parser = argparse.ArgumentParser(description='Calculate ratio of metric for each application')
    parser.add_argument('file1', type=str, help='Path to the first dataset file')
    parser.add_argument('file2', type=str, help='Path to the second dataset file')
    parser.add_argument('metric', type=str, choices=['seconds', 'mops'], help='Metric to calculate ratio (seconds or mops)')
    args = parser.parse_args()

    data1 = read_data(args.file1)
    data2 = read_data(args.file2)

    ratios = calculate_ratio(data1, data2, args.metric)

    print("Ratio of metric for each application:")
    for app_name, ratio in ratios.items():
        print(f"{app_name}: {ratio:.2f}")

    plot_ratios(ratios)

if __name__ == "__main__":
    main()
