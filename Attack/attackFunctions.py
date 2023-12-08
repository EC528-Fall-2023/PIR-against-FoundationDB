import random
import math
import pandas as pd
from final_Mp import df_matrix

def get_cipher_values(file_path):
    custom_delimiter = 'test '
    row_delimiter = 'Key,Value'
    data = []
    with open(file_path, 'r') as file:
        content = file.read()
        # Split content by row delimiter
        rows = content.split(row_delimiter)
        for row in rows:
            # Split each row by column delimiter
            columns = row.strip().split(custom_delimiter)
            # Remove empty columns and append to data
            filtered_columns = [col for col in columns if col.strip()]
            data.append(filtered_columns)
    df = pd.DataFrame(data, columns=['Key', 'Value'])
    keys = df['Value'].values.tolist()
    keys.pop(0)
    return(keys)

def get_unique_keywords():
    file_path = 'final_unique_keywords.csv'
    data = []
    with open(file_path, 'r') as file:
        for line in file:
            data.append(line.strip())
    return data

# Algorithm 1 - Optimizer
def optimizer(V, D, K, Mp, Mc):
    valList = D.copy()
    initStater = {}

    # Assign random mappings between cipher and plain keywords not in the background
    remaining_cipher = [var for var in V if var not in K.keys()]
    for var in remaining_cipher:
        val = random.choice(valList)
        initStater[var] = val
        valList.remove(val)

    # Initialize with known assignments from background knowledge
    initStater.update(K)
    
    return anneal(initStater, D, Mp, Mc, initTemperature, coolingRate, rejectThreshold)

def anneal(initState, D, Mp, Mc, initTemperature, coolingRate, rejectThreshold):
    currentState = initState.copy()
    succReject = 0
    currT = initTemperature

    while currT != 0 and succReject < rejectThreshold:
        currentCost = 0
        nextCost = 0
        nextState = currentState.copy()

        cipher_var = random.choice(list(nextState.keys()))
        current_plain = nextState[cipher_var]
        next_plain = random.choice([val for val in D if val != current_plain])

        del nextState[cipher_var]
        nextState[cipher_var] = next_plain

        print(currentState)
        print(nextState)
        if any(next_plain == v for v in initState.values()):
            z = [k for k, v in initState.items() if v == next_plain][0]
            del nextState[z]
            nextState[z] = current_plain

        for i in Mc:
            for j in Mc[i]:
                k = currentState.get(i)
                l = currentState.get(j)
                k_prime = nextState.get(i)
                l_prime = nextState.get(j)
                currentCost += (Mc[i][j] - Mp[k][l])**2
                nextCost += (Mc[i][j] - Mp[k_prime][l_prime])**2

        E = nextCost - currentCost
        print("nextCost:", nextCost)
        print("currentCost:", currentCost)
        if E < 0 or random.uniform(0, 1) < math.exp(-E / currT):
            currentState = nextState.copy()
            succReject = 0
        else:
            succReject += 1

        currT *= coolingRate
    return currentState


# Example Usage
D = get_unique_keywords()
V = ['cipher_var1', 'cipher_var2', 'cipher_var3']  # List of cipher variables
K = {}  # Known assignments
Mp = df_matrix
Mc = {'cipher_var1': {'cipher_var1': 0, 'cipher_var2': 0.2, 'cipher_var3': 0.2},  # Pair similarity matrices
      'cipher_var2': {'cipher_var1': 0.3, 'cipher_var2': 0, 'cipher_var3': 0.5},
      'cipher_var3':{'cipher_var1':0.4, 'cipher_var2': 0.6, 'cipher_var3': 0}}

initTemperature = 1000
coolingRate = 0.99
rejectThreshold = 1000
result = optimizer(V, D, K, Mp, Mc)
print(result)