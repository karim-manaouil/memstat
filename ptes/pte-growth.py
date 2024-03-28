import re
import matplotlib.pyplot as plt

# Regular expression pattern to extract page table size from between parentheses
pattern = r'\((\d+)\s+MiB\)'

# Lists to store time and page table size data
timestamps = []
page_table_sizes = []

# Read the file and extract data
with open('pte_growth', 'r') as file:
    for line in file:
        # Extract timestamp
        timestamp = float(line.split()[0][1:-1])  # Extracting the timestamp from the line
        timestamps.append(timestamp)

        # Extract page table size using regular expression
        match = re.search(pattern, line)
        if match:
            page_table_size = int(match.group(1))
            page_table_sizes.append(page_table_size)
        else:
            page_table_sizes.append(None)  # Append None if page table size is not found

# Plotting
plt.plot(timestamps, page_table_sizes, marker='o', linestyle='-')
plt.xlabel('Timestamp')
plt.ylabel('Page Table Size (MiB)')
plt.title('Page Table Size Over Time')
plt.grid(True)

for i in range(len(timestamps)):
    plt.text(timestamps[i], page_table_sizes[i], f'{page_table_sizes[i]}', fontsize=8, ha='right')

plt.show()

