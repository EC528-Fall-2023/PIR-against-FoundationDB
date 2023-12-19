import os
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def read_files(file_paths, testing_var):
    x_values = []  # List to store x coordinates
    y_values = []  # List to store y coordinates

    for idx, file_path in enumerate(file_paths):
        try:
            parts = file_path.split('_')
            part = parts[testing_var+1]
            if (testing_var+1 == 4):
                part = part.split('.')[0]
            with open(file_path, 'r') as file:
                y_values.extend([float(line.strip()) * 1000 for line in file.readlines()])
                x_values.extend([int(part) for _ in range(int(10))])
        except FileNotFoundError:
            print(f"File not found: {file_path}")
        except ValueError:
            print(f"Invalid decimal number in file: {file_path}")

    return x_values, y_values

def plot_graph(x_values, y_values, mode, testing_var):

    print(len(x_values))
    print(len(y_values))
    means, stds = [], []
    for unique_x in np.unique(x_values):
        values = [y_values[i] for i in range(len(x_values)) if x_values[i] == unique_x]
        means.append(np.mean(values))
        stds.append(np.std(values) * 2)

    plt.figure()
    plt.scatter(np.unique(x_values), means, marker='^', label='Data')
    plt.scatter(x_values, y_values)
    plt.errorbar(np.unique(x_values), means, yerr=stds, fmt='none', capsize=5, label='Error')
    if testing_var == 1:
        plt.xlabel('Block Size (B)')
        plt.title(f'{mode} Times vs. Block Size')
    elif testing_var == 2:
        plt.xlabel('Blocks per Bucket')
        plt.title(f'{mode} Times vs. Blocks per Bucket')
    elif testing_var == 3:
        plt.xlabel('Tree Levels')
        plt.title(f'{mode} Times vs. Tree Levels')
    elif testing_var == 4:
        plt.xlabel('Tree Fullness')
        plt.title(f'{mode} Times vs. Tree Fullness')
    plt.ylabel('Time (ms)')
    plt.savefig(f'images/{mode}_{testing_var}_graph.png')

def main():
    # Example list of file paths (you can replace this with your file paths)
    folder = 'bmk_out'
    mode = input("mode (put, get, clear, startup): ")
    testing_var = int(input("what are you testing? 1=bytes, 2=blocks, 3=levels 4=fullness "))

    #file_paths = ['bmk_out/put_1024_1_8_10.txt', 'bmk_out/put_16384_1_8_10.txt', 'bmk_out/put_32768_1_8_10.txt', 'bmk_out/put_65536_1_8_10.txt']

    BYTES = [1024, 16384, 32768, 65536]
    BLOCKS = [1, 16, 32, 48]
    LEVELS = [8, 12, 16, 20]
    FULLNESS = [100, 255]

    testing_vars = []

    if testing_var == 1:
        testing_vars = BYTES
    elif testing_var == 2:
        testing_vars = BLOCKS
    elif testing_var == 3:
        testing_vars = LEVELS
    elif testing_var == 4:
        testing_vars = FULLNESS


    file_paths = [os.path.join(folder, f"{mode}_{1024 if testing_var != 1 else var}_{1 if testing_var != 2 else var}_{8 if testing_var != 3 else var}.txt") for var in testing_vars]
    file_paths = ["bmk_out/without.txt"]
    x_values, y_values = read_files(file_paths, testing_var)
    plot_graph([1 for _ in range(len())], y_values, mode, testing_var)

if __name__ == "__main__":
    main()
