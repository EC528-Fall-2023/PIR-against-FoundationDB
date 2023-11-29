import pandas as pd
from nltk.tokenize import word_tokenize
from nltk.stem import PorterStemmer
from nltk.probability import FreqDist
import numpy as np

# Reads the dataset and removes irrelevant columns
unsorted_data = pd.read_csv('spam_ham_dataset.csv')
unsorted_data = unsorted_data.drop(columns=['col2', 'col3'])

# Sorts the data by the first column
sorted_data = unsorted_data.sort_values(by='col1')

# Cleans the text data
cleaned_strings = sorted_data['text'].str.replace('[^a-zA-Z \n\']', ' ')
cleaned_strings = cleaned_strings.str.replace('\s+', ' ')
cleaned_strings = cleaned_strings.str.replace('\s*\'\s*', '\'')
sorted_data['text'] = cleaned_strings

# Splits the data into the test set and training set
test_set = sorted_data.iloc[:200, :]
training_set = sorted_data.iloc[200:, :].reset_index(drop=True)
training_set['ID'] = np.arange(len(training_set))

# Creates matrix M for the probability of two given keywords appearing together
documents = [word_tokenize(text) for text in training_set['text']]

# Converts to an array of words
doc_string = [word for doc in documents for word in doc]

# Finds all unique words within the training set
unique_keywords = list(set(doc_string))

# Stems the words using Porter Stemmer
ps = PorterStemmer()
stem_words = [ps.stem(word) for word in unique_keywords]
unique_stem_words = list(set(stem_words))

# Creates a blank template for matrix M
M = np.zeros((len(unique_stem_words), len(unique_stem_words)))

# Iterates through each possible pair of unique keywords
count = 0
for i in range(len(unique_stem_words)):
    for j in range(len(unique_stem_words)):
        # Finds the probability each word appears in any document
        prob_i = sum(any(word in doc for doc in documents) for word in unique_stem_words[i]) / len(documents)
        prob_j = sum(any(word in doc for doc in documents) for word in unique_stem_words[j]) / len(documents)

        # Finds the probability of both words appearing
        co_occurrence_count = sum(any(word in doc for doc in documents) for word in (unique_keywords[i], unique_keywords[j]))
        prob_both = co_occurrence_count / len(documents)

        # Calculates the expected probability of both appearing in any given document
        M[i, j] = prob_i * prob_j * prob_both

        # Checking progress
        count += 1
        print(f'Complete: {count}/{len(unique_keywords)**2}')
