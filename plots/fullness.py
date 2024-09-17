import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import re

# Function to parse log data from a file

plt.rcParams.update({'font.size': 12})


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
file_path = './logs/waste-management-10hr-5bin.txt'

# Parse the log file
data = parse_log_file(file_path)

# Plotting
plt.figure(figsize=(8, 5))

color_map = {
    1: "r",
    2: "g",
    3: "b",
    4: "c",
    5: "m"
}

data_int = {}

lst = [None, None, None, None, None]

for id, (id_str, values) in enumerate(data.items()):
    print(id_str)
    times = [x[0] for x in values]
    readings = [x[1] for x in values]

    lst[int(id_str.split(":")[1]) - 1] = (times, readings, id_str)

for item in lst:
    plt.plot(item[0], item[1], linestyle='-',
             label=item[2], color=mcolors.BASE_COLORS[color_map[int(item[2].split(":")[1])]])

plt.xlim(0, 600)
plt.ylim(0, 100)
plt.xlabel('Time [$m$]')
plt.ylabel('Fullness [$\%$]')
plt.title('Fullness of 5 Station Bins over Simulation Time')
plt.tight_layout()
plt.legend(title='IDs')
plt.grid(True)
plt.savefig("./plots/out/fullness.pdf")
