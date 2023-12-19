import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import statistics

file_name = '../bmk_out/without.txt'

# Read numbers from file into a list
with open(file_name, 'r') as file:
    numbers = [float(line.strip()) * 1000 for line in file]

mean_value = statistics.mean(numbers)
std_value = statistics.stdev(numbers) * 2

plt.figure()
plt.scatter([0 for _ in numbers], numbers)
plt.errorbar([0], [mean_value], yerr=[std_value], fmt='none', capsize=5, label='Error')
plt.xlabel('')
plt.title('Times vs Accesses to FDB Without Path ORAM')
plt.ylabel('Time (ms)')
plt.savefig('../images/without.png')
