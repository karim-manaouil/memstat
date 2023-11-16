import csv
import sys
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

# Check if the correct number of arguments is provided
if len(sys.argv) != 2:
    print("Usage: python script_name.py filename.csv")
    sys.exit(1)

# Get the filename from the command line arguments
filename = sys.argv[1]

# Initialize lists to store X and Y values from CSV
x_values = []
y_percentages = []

# Read data from CSV file and convert Y values to float percentages
with open(filename, 'r') as csvfile:
    csvreader = csv.reader(csvfile)
    for row in csvreader:
        # Assuming X values are in the first column (index 0) and Y values (as percentages) in the second column (index 1) of each row
        x_values.append(row[0])
        y_percentages.append(float(row[1]))  # Convert percentage to float

# Create a bar plot for X values vs Y values
plt.bar(x_values, y_percentages, color='blue')

# Customize the y-axis labels to show percentages
plt.gca().yaxis.set_major_formatter(ticker.PercentFormatter(symbol='%', decimals=2))

# Add data labels on top of the bars
for i, v in enumerate(y_percentages):
    plt.text(i, v + 0.02, '{:.2%}'.format(v), ha='center', va='bottom')

# Customize the plot
plt.title('Memory access latency')
plt.xlabel('Latency intervals')
plt.ylabel('Percentage')
plt.xticks(rotation='vertical')  # Rotate x-axis labels for better visibility

# Show the plot
plt.tight_layout()
plt.show()

