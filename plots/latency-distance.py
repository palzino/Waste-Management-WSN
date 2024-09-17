import dataclasses
import matplotlib.pyplot as plt
import numpy as np


@dataclasses.dataclass
class Coordinate:
    x: float
    y: float


def distance(p1: Coordinate, p2: Coordinate) -> float:
    return (((p2.x - p1.x) ** 2) + ((p2.y - p1.y) ** 2)) ** 0.5


plt.rcParams.update({'font.size': 12})
plt.figure(figsize=(8, 5))

s1 = Coordinate(78.958, 75.504)
b2 = Coordinate(0.022, 50.862)
b3 = Coordinate(19.979, 40.326)
b4 = Coordinate(0.001, 75.126)
b5 = Coordinate(4.653, 99.905)
b6 = Coordinate(17.881, 70.004)
b7 = Coordinate(24.268, 106.969)
b8 = Coordinate(37.39, 90.08)
b9 = Coordinate(48.387, 108.566)
b10 = Coordinate(49.642, 78.593)
b11 = Coordinate(49.82, 59.851)
b12 = Coordinate(71.04, 90.915)
b13 = Coordinate(92.364, 109.729)
b14 = Coordinate(89.905, 80.421)
b15 = Coordinate(109.803, 79.928)
b16 = Coordinate(123.647, 100.239)
b17 = Coordinate(109.608, 40.535)
b18 = Coordinate(130.052, 65.131)
b19 = Coordinate(130.047, 34.303)
b20 = Coordinate(129.098, 10.975)
b21 = Coordinate(100.219, 9.047)
b22 = Coordinate(161.207, 67.874)

distances = [
    distance(b2, s1),  # 2
    distance(b3, s1),  # 3
    distance(b4, s1),  # 4
    distance(b5, s1),  # 5
    distance(b6, s1),  # 6
    distance(b7, s1),  # 7
    distance(b8, s1),  # 8
    distance(b9, s1),  # 9
    distance(b10, s1),  # 10
    distance(b11, s1),  # 11
    distance(b12, s1),  # 12
    distance(b13, s1),  # 13
    distance(b14, s1),  # 14
    distance(b15, s1),  # 15
    distance(b16, s1),  # 16
    distance(b17, s1),  # 17
    distance(b18, s1),  # 18
    distance(b19, s1),  # 19
    distance(b20, s1),  # 20
    distance(b21, s1),  # 21
    distance(b22, s1),  # 22
]

# As calculated in latency.py

latencies = [
    162.233606557377046,  # 2
    76.8130081300813,  # 3
    94.42682926829268,  # 4
    160.22633744855966,  # 5
    86.26970954356847,  # 6
    99.15040650406505,  # 7
    116.21224489795918,  # 8
    45.447154471544714,  # 9
    113.33877551020409,  # 10
    52.38211382113821,  # 11
    51.764227642276424,  # 12
    60.4349593495935,  # 13
    106.06147540983606,  # 14
    123.52697095435684,  # 15
    112.27868852459017,  # 16
    134.30165289256198,  # 17
    389.149377593361,  # 18
    85.82520325203252,  # 19
    132.4688796680498,  # 20
    214.9,  # 21
    213.04508196721312,  # 22
]

plt.plot(distances, latencies, 'x', color="black")

coefficients = np.polyfit(distances, latencies, 1)
linear_fit = np.poly1d(coefficients)

# Generate x values for the line of best fit
x_fit = np.linspace(min(distances), max(distances), 100)
y_fit = linear_fit(x_fit)

# Plot the line of best fit
plt.plot(x_fit, y_fit, '--', label='Line of Best Fit', color='red')


plt.xlim(0, 100)
plt.ylim(0, 400)
plt.xlabel('Distance from Root Node [$m$]')
plt.ylabel('Average Delay [$ms$]')
plt.title('Average Delay over Distance from Root Node')
plt.tight_layout()
plt.savefig("./plots/out/latency-distance.pdf")
