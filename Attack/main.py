import pandas as pd
from nltk.stem import PorterStemmer
import numpy as np

# Reads the dataset
unsorted_data = pd.read_csv('spam_ham_dataset.csv')

# Removes irrelevant columns (assuming column indexing starts from 0)
unsorted_data = unsorted_data.drop(unsorted_data.columns[[1, 3]], axis=1)

# Sorts the data based on the first column (assuming column indexing starts from 0)
sorted_data = unsorted_data.sort_values(by=unsorted_data.columns[0])
print(sorted_data.columns)

# Extracts the 'text' column from sorted_data
strings = sorted_data['text']

# Converts to string type
strings = strings.astype(str)

# Cleans the strings using regular expressions
cleaned_strings = strings.str.replace(r'[^a-zA-Z \n\']', ' ')
sorted_data['text'] = cleaned_strings

# Replaces newline characters with a space
sorted_data['text'] = sorted_data['text'].replace('\n', ' ')

# Removes extra spaces and single quotes
cleaned_strings = sorted_data['text'].str.replace(r'\s+', ' ')
cleaned_strings = cleaned_strings.str.replace(r'\s*\'\s*', "'")

# Assigns cleaned strings back to the DataFrame
sorted_data['text'] = cleaned_strings

# Splits the data into test set and training set
test_set = sorted_data.iloc[:200, :]
training_set = sorted_data.iloc[200:, :]

# Sets the 'ID' column to start at 0 for the training set
training_set['ID'] = range(len(training_set))

# Concatenate all text in the training set into a single string
all_text = ' '.join(training_set['text'])

# Split the text into words
all_words = all_text.split()

# Find all unique words within the training set
unique_keywords = list(set(all_words))

# Apply Porter stemming algorithm to the unique words
porter = PorterStemmer()
stemmed_words = [porter.stem(word) for word in unique_keywords]

# Create a blank template for matrix M
M = np.zeros((len(stemmed_words), len(stemmed_words)))

total_combinations = len(unique_keywords)**2

for i in range(len(stemmed_words)):
    for j in range(len(stemmed_words)):
        # Find the probability each word appears in any document
        prob_i = sum(stemmed_words in text for text in training_set['text']) / len(training_set)
        prob_j = prob_i  # Since the expression is the same, no need to recalculate

        # Find the probability of both words appearing
        co_occurrence_count = sum(
            word_i in text and word_j in text
            for text in training_set['text']
            for word_i in unique_keywords
            for word_j in unique_keywords
        )
        prob_both = co_occurrence_count / len(training_set)

        # Calculate the expected probability of both appearing in any given document
        M[i, j] = prob_i * prob_j * prob_both
# Convert the matrix M to a Pandas DataFrame
df_matrix = pd.DataFrame(M, index=stemmed_words, columns=stemmed_words)

# Save the DataFrame to a CSV file
df_matrix.to_csv('output_matrix.csv')