import subprocess
import time
import json
import signal
import sys
import argparse
import matplotlib.pyplot as plt
import numpy as np

STOP = 0
DATA_FILE = "data/vmstat_data.json"

scan_interval = 0.01 # 10ms

def sigterm_handler(signum, frame):
    global STOP
    print("SIGTERM received. Exiting gracefully...")
    STOP = 1

def process_output_line(line, data):
    elements = line.split()
    if len(elements) > 0: 
        metric = elements[0]
        if not metric in data:
            data[metric] = []
            data[metric].append(float(elements[1]))
        else:
            # We want relative values
            data[metric].append(float(elements[1]) - data[metric][0])


def run_command_and_parse():
    global STOP
    data = {}

    while STOP == 0:
        command_output = subprocess.run(['cat', '/proc/vmstat'],
                                capture_output=True, text=True)
        output_lines = command_output.stdout.split('\n')
    
        for line in output_lines:
            process_output_line(line, data)

        time.sleep(scan_interval) 
    
    return data

def plot_metrics(metrics, data):
    for metric in metrics:
        if not metric in data:
            print(metric + " doesn't exist!!")
            exit(0)
        v = data[metric][1:] # [0] is used to calculate relative values 
        plt.plot(v, label=metric)

    plt.xlabel("Time")
    plt.ylabel("Pages")
    plt.title("Evolution")
    plt.legend()
    plt.show()

def plot(metrics):
    with open(DATA_FILE, "r") as json_file:
        data = json.load(json_file)
    
    plot_metrics(metrics, data)
    
def run():
    signal.signal(signal.SIGINT, sigterm_handler)
    data = run_command_and_parse()

    with open(DATA_FILE, 'w') as file:
        json.dump(data, file)
    
    print("Data saved to " + DATA_FILE)

def main():
    global DATA_FILE

    parser = argparse.ArgumentParser(description='vmstat processor')
    parser.add_argument('--stat', required=True, type=str, help='Stats file')
    parser.add_argument('--plot', nargs="+", type=str, help='Plot metrics')
    parser.add_argument('--run', action='store_true', help='Start monitoring')

    args = parser.parse_args()
 
    DATA_FILE = "data/vmstat_" + args.stat + ".json"

    if args.run:
        run()
    elif args.plot:
        plot(args.plot)
main()
