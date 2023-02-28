import subprocess
import time
import json
import signal
import sys
import argparse
import matplotlib.pyplot as plt
import numpy as np

COLS = 3
STOP = 0
DATA_FILE = "data/numastat_data.json"

scan_interval = 0.01

def sigterm_handler(signum, frame):
    global STOP
    print("SIGTERM received. Exiting gracefully...")
    STOP = 1

def process_output_line(line, data):
    elements = line.split()
    if len(elements) > 0: 
        metric = elements[0]
        if not metric in data:
            data[metric] = {}
        for node in range(1, COLS + 1):
            if not node in data[metric]:
                data[metric][node] = []
            data[metric][node].append(float(elements[node]))

def run_command_and_parse():
    global STOP
    data = {}

    while STOP == 0:
        command_output = subprocess.run(['numastat', '-m'],
                                capture_output=True, text=True)
        output_lines = command_output.stdout.split('\n')[-32:]

        for line in output_lines:
            process_output_line(line, data)

        time.sleep(scan_interval) 
    
    return data

def plot_metric(metric, data):
    for node in data[metric]:
        v = data[metric][node]
        plt.plot(v, label="node " + node)
    
    plt.xlabel("Time")
    plt.ylabel("MiB")
    plt.title("Evolution of " + metric)
    plt.legend()
    plt.show()

def plot(metric):
    with open(DATA_FILE, "r") as json_file:
        data = json.load(json_file)
    
    if not metric in data:
        print(metric + " doesn't exist!!")
        exit(1)
    
    plot_metric(metric, data)
    
def run():
    signal.signal(signal.SIGINT, sigterm_handler)
    data = run_command_and_parse()

    with open(DATA_FILE, 'w') as file:
        json.dump(data, file)
    
    print("Data saved to " + DATA_FILE)

def main():
    parser = argparse.ArgumentParser(description='numastat processor')

    parser.add_argument('--plot', type=str, help='Plot a metric')
    parser.add_argument('--run', action='store_true', help='Start monitoring')

    args = parser.parse_args()
 
    if args.run:
        run()
    elif args.plot:
        plot(args.plot)
main()
