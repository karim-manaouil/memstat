import subprocess
import time
import json
import signal
import sys
import argparse
import matplotlib.pyplot as plt
import numpy as np

STOP = 0
DATA_FILE = "data/cpustat_data.json"

scan_interval = 0.1

def sigterm_handler(signum, frame):
    global STOP
    print("SIGTERM received. Exiting gracefully...")
    STOP = 1

def run_command_and_parse():
    global STOP
    data = []

    last_idle = last_total = 0
    while STOP == 0:
        with open('/proc/stat') as f:
            fields = [float(column) for column in f.readline().strip().split()[1:]]
            idle, total = fields[3], sum(fields)
            idle_delta, total_delta = idle - last_idle, total - last_total
            last_idle, last_total = idle, total
            utilisation = 100.0 * (1.0 - idle_delta / total_delta)
            data.append(utilisation)

        time.sleep(scan_interval) 
    
    return data

def plot_metric(data):
    plt.plot(data)
    
    plt.xlabel("Time")
    plt.ylabel("perc %")
    plt.title("CPU utilisation")
    plt.legend()
    plt.show()

def plot():
    with open(DATA_FILE, "r") as json_file:
        data = json.load(json_file)
    
    plot_metric(data)
    
def run():
    signal.signal(signal.SIGINT, sigterm_handler)
    data = run_command_and_parse()

    with open(DATA_FILE, 'w') as file:
        json.dump(data, file)
    
    print("Data saved to " + DATA_FILE)

def main():
    global DATA_FILE

    parser = argparse.ArgumentParser(description='numastat processor')
    parser.add_argument('--stat', required=True, type=str, help='Stats file')
    parser.add_argument('--plot', action='store_true', help='Plot CPU utilisation')
    parser.add_argument('--run', action='store_true', help='Start monitoring')

    args = parser.parse_args()

    DATA_FILE = "data/cpustat_" + args.stat + ".json"

    if args.run:
        run()
    elif args.plot:
        plot()
main()
