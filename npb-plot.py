import sys
import matplotlib.pyplot as plt

# Function to parse the data file
def parse_data_file(filename):
    with open(filename, 'r') as file:
        lines = file.readlines()

    numa_nodes = {}
    current_node = None

    for line in lines:
        if line.startswith("NUMA node"):
            current_node = line.strip()
            numa_nodes[current_node] = {}
        else:
            parts = line.strip().split(',')
            program_name = parts[0]
            metrics = parts[1:]
            metrics_dict = {}
            for metric in metrics:
                metric_name, metric_value = metric.split('=')[0], float(metric.split('=')[1])
                metrics_dict[metric_name] = metric_value
            numa_nodes[current_node][program_name] = metrics_dict

    return numa_nodes

# Function to normalize the data based on user input (seconds or mops)
def normalize_data(numa_nodes, normalization_metric):
    normalized_nodes = {}
    base_node = "NUMA node 0"

    for node, data in numa_nodes.items():
        normalized_nodes[node] = {}
        for program, metrics in data.items():
            normalized_metrics = {}
            base_metric = numa_nodes[base_node][program]
            for metric_name, metric_value in metrics.items():
                if metric_name == normalization_metric:
                    normalized_value = metric_value / base_metric[metric_name]
                    normalized_metrics[metric_name] = normalized_value
                else:
                    normalized_metrics[metric_name] = metric_value
            normalized_nodes[node][program] = normalized_metrics

    return normalized_nodes

# Function to plot the slowdown for each program on each NUMA node
def plot_slowdown(normalized_nodes):
    programs = sorted(list(normalized_nodes["NUMA node 0"].keys()))
    numa_node_labels = sorted(list(normalized_nodes.keys()))

    plt.figure(figsize=(12, 6))

    for program in programs:
        slowdowns = [normalized_nodes[node][program]["seconds"] for node in numa_node_labels]
        plt.plot(numa_node_labels, slowdowns, marker='o', label=program)

    plt.xlabel('NUMA node')
    plt.ylabel('Slowdown (Normalized to NUMA node 0)')
    plt.title('Slowdown for each program on each NUMA node')
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.show()

# Main function
def main():
    if len(sys.argv) != 3:
        print("Usage: python script.py <filename> <normalization_metric>")
        print("<normalization_metric> should be 'seconds' or 'mops'")
        return

    filename = sys.argv[1]
    normalization_metric = sys.argv[2].lower()

    if normalization_metric not in ['seconds', 'mops']:
        print("Invalid normalization metric! Please enter 'seconds' or 'mops'.")
        return

    numa_nodes = parse_data_file(filename)
    normalized_nodes = normalize_data(numa_nodes, normalization_metric)
    plot_slowdown(normalized_nodes)

if __name__ == "__main__":
    main()
