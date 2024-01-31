import subprocess
import time
import json
import signal
import sys
import argparse
import matplotlib.pyplot as plt
import numpy as np

STOP = 0
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

def compare_metrics(metrics, data1, data2):
    for metric in metrics:
        if not metric in data1 or metric not in data2:
            print(metric + " doesn't exist!!")
            exit(0)

        v1 = data1[metric][1:] # [0] is used to calculate relative values
        v2 = data2[metric][1:] # [0] is used to calculate relative values

        plt.plot(v1, label=f'{metric} - src', linestyle='--')
        plt.plot(v2, label=f'{metric} - dst', linestyle='--')

    plt.xlabel("Time")
    plt.ylabel("Pages")
    plt.title("Evolution")
    plt.legend()
    plt.show()

def plot(metrics, file):
    with open(file, "r") as json_file:
        data = json.load(json_file)
    
    plot_metrics(metrics, data)

def compare(metrics, file1, file2):
    with open(file1, "r") as json1:
        data1 = json.load(json1)

    with open(file2, "r") as json2:
        data2 = json.load(json2)

    compare_metrics(metrics, data1, data2)

def run(file):
    signal.signal(signal.SIGINT, sigterm_handler)
    data = run_command_and_parse()

    with open(file, 'w') as file:
        json.dump(data, file)
    
    print("Data saved to " + file)

def main():
    parser = argparse.ArgumentParser(description='vmstat processor')
    parser.add_argument('--stat', nargs="+", required=True, type=str, help='Stats file')
    parser.add_argument('--plot', nargs="+", type=str, help='Plot metrics')
    parser.add_argument('--run', action='store_true', help='Start monitoring')

    args = parser.parse_args()
 
    file1 = "data/vmstat_" + args.stat[0] + ".json"

    if args.run:
        run(file1)
    elif args.plot:
        if len(args.stat) == 1:
            plot(args.plot, file1)
        else:
            file2 = "data/vmstat_" + args.stat[1] + ".json"
            compare(args.plot, file1, file2)
main()
