import re
import dataclasses
import enum
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import numpy as np

plt.rcParams.update({'font.size': 12})

color_map = {
    1: "r",
    2: "g",
    3: "b",
    4: "c",
    5: "m"
}


class LogType(enum.Enum):
    model_params = "model_params"
    new_log = "new_log"
    buffer_size = "buffer_size"
    buffer_reset = "buffer_reset"
    sent_fullness_log = "sent_fullness_log"
    updated_log = "updated_log"
    will_empty = "will_empty"
    started_emptying = "started_emptying"
    sent_empty_signal = "sent_empty_signal"
    emptying = "emptying"
    emptied = "emptied"


@dataclasses.dataclass
class Log:
    id: int
    time_ms: int
    type: LogType
    data: list[str]


@dataclasses.dataclass
class Bin:
    id: int
    ip: str
    diff: int = 0
    diff_count: int = 0


def time_to_milliseconds(time_str):
    hours, minutes, seconds = time_str.split(':')
    seconds, milliseconds = seconds.split('.')
    total_milliseconds = (int(hours) * 3600000) + (int(minutes)
                                                   * 60000) + (int(seconds) * 1000) + int(milliseconds)
    return total_milliseconds


def parse_file(path) -> tuple[list[Log], list[Bin]]:
    # Open file
    file = open(path, "r")
    data = file.read()
    log_pattern_log = r"(\d{1,2}:\d{2}:\d{2}\.\d{3})\s+ID:(\d+)\s+\[INFO:\s+\w+\s+\]\s+\[(\w+)\]\{([^\}]*)\}"
    # Get all matches
    matches_logs = re.findall(log_pattern_log, data)
    # Get all ips
    log_pattern_ip = r"(\d{1,2}:\d{2}\.\d{3})\s+ID:(\d+)\s+\[INFO:\s+\w+\s+\]\s+Tentative link-local IPv6 address:\s+([a-fA-F0-9:]+)"
    matches_ips = re.findall(log_pattern_ip, data)
    # Output array of parsed data
    out: list[Log] = []
    out_bins: list[Bin] = []
    # Iterate through lines and create objects
    for matched_line in matches_logs:
        time_str, id, type, data = matched_line
        time_ms = time_to_milliseconds(time_str)
        if type in LogType._value2member_map_:
            type_enum = LogType(type)
            out.append(Log(int(id), time_ms, type_enum, data.split(',')))
        else:
            raise Exception(f"Invalid log type in data: {matched_line}")
    for matched_ip in matches_ips:
        time_str, id_value, ip_address = matched_ip
        if int(id_value) == 1:
            continue  # Skip the sink
        out_bins.append(Bin(int(id_value), ip_address))
    return out, out_bins


logs, bins = parse_file("./logs/transmission-delays-5hr-21bin.txt")

for idx, bin in enumerate(bins):

    recent_reading = 0
    recent_time = None

    for log in [log for log in logs if log.id == bin.id or log.id == 1]:
        # On the first time the bin will send it's current fullness
        if log.type == LogType.model_params:
            recent_reading = int(log.data[0])
        # On each incremental time, the bin will send its fullness log
        elif log.type == LogType.sent_fullness_log:
            recent_reading = int(log.data[0])
            recent_time = log.time_ms  # Record time this was sent
        # When the sink node receives the record
        elif (log.type == LogType.updated_log or log.type == LogType.new_log) and bin.ip.split("::")[1] == log.data[0].split("::")[1] and log.id == 1:
            if recent_time == None:
                # Skip if no recent reading, not been sent yet or the reading received doesn't match what was recently sent
                continue
            time_difference = log.time_ms - recent_time
            bins[idx].diff_count += 1
            bins[idx].diff += time_difference

    print(bin.id)
    print(bins[idx].diff / bins[idx].diff_count)

plt.figure(figsize=(8, 5))


for bin in sorted(bins, key=lambda bin: bin.id):
    # Skip the sink
    if bin.id == 1:
        continue
    plt.bar(bin.id, bin.diff / bin.diff_count,
            color=mcolors.BASE_COLORS[color_map[((bin.id - 2) % 5 + 1)]])


plt.xticks(np.arange(2, 23, step=1))

plt.rc('axes', axisbelow=True)


plt.xlabel('Node ID')
plt.ylabel('Average Delay [$ms$]')
plt.title('Average Time Taken for New Readings to Reach the Root Node')
plt.tight_layout()
plt.savefig("./plots/out/latency.pdf")
