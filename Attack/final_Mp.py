import pandas as pd
from nltk.stem import PorterStemmer
import numpy as np
from sklearn.feature_extraction.text import CountVectorizer
# Reads the dataset
unsorted_data = pd.read_csv('spam_ham_dataset.csv')

# Removes irrelevant columns (assuming column indexing starts from 0)
unsorted_data = unsorted_data.drop(unsorted_data.columns[[1, 3]], axis=1)

# Sorts the data based on the first column (assuming column indexing starts from 0)
sorted_data = unsorted_data.sort_values(by=unsorted_data.columns[0])
# print(sorted_data.columns)

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

file_path = 'final_unique_keywords.csv'
unique_keywords = []
with open(file_path, 'r') as file:
    for line in file:
        unique_keywords.append(line.strip())

porter = PorterStemmer()
stemmed_words = [porter.stem(word) for word in unique_keywords]

stemmed_words1 = stemmed_words[:len(stemmed_words)//2]
stemmed_words2 = stemmed_words[len(stemmed_words)//2:]

# Create a DataFrame with a single column 'list1'
df_keywords1 = pd.DataFrame({'Stemmed_Words': stemmed_words1})

# Save the DataFrame to a CSV file
df_keywords1.to_csv('list1.csv', index=False)

# Create a DataFrame with a single column 'list2'
df_keywords2 = pd.DataFrame({'Stemmed_Words': stemmed_words2})

# Save the DataFrame to a CSV file
df_keywords2.to_csv('list2.csv', index=False)

total_combinations = len(unique_keywords)**2

# Assuming sorted_data['text'] contains preprocessed text data
texts = training_set['text']

# Create CountVectorizer with desired parameters
vectorizer = CountVectorizer(analyzer='word', vocabulary=unique_keywords, tokenizer=lambda text: text.split())

# Fit and transform the text data to get the co-occurrence matrix
X = vectorizer.fit_transform(texts)
co_occurrence_matrix = (X.T * X)
co_occurrence_matrix = co_occurrence_matrix / len(texts)

# Convert the matrix M to a Pandas DataFrame
df_matrix = pd.DataFrame.sparse.from_spmatrix(co_occurrence_matrix, index=unique_keywords, columns=unique_keywords)
# Save the DataFrame to a CSV file
print(df_matrix)