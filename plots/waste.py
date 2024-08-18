import matplotlib.pyplot as plt
import re

# Function to parse log data from a file


def parse_log_file(file_path):
    data = {}
    pattern = re.compile(r'\[sent_fullness_log\]\{(\d+),(\d+)\}')

    with open(file_path, 'r') as file:
        for line in file:
            if '[sent_fullness_log]' in line:
                match = pattern.search(line)
                if match:
                    id_match = re.search(r'ID:\d+', line)
                    if id_match:
                        id_str = id_match.group()
                        value = int(match.group(1))
                        time = int(match.group(2))
                        if id_str in data:
                            data[id_str].append((time, value))
                        else:
                            data[id_str] = [(time, value)]

    return data


# File path to the log file
file_path = './logs/test.txt'

# Parse the log file
data = parse_log_file(file_path)

# Plotting
plt.figure(figsize=(12, 8))

for id_str, values in data.items():
    times = [x[0] for x in values]
    readings = [x[1] for x in values]
    plt.plot(times, readings, linestyle='-', label=id_str)

plt.xlabel('Time (integer)')
plt.ylabel('First Number in {,} following [sent_fullness_log]')
plt.title('Readings of Each Bin Over Time')
plt.legend(title='IDs')
plt.grid(True)
plt.savefig("test.png")
