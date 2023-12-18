import pandas as pd
import numpy as np
from sklearn.feature_extraction.text import CountVectorizer

# Replace 'cipher_data.csv' with the actual file name that contains your cipher text data.
cipher_data = pd.read_csv('cipher_data.csv')

# Assuming 'Block_ID' is the column with block IDs and 'Cipher_Words' is the column with cipher words
# Clean the cipher words column using regular expressions by cleaning the cipher words that are separated by spaces and cleaned similarly to plain text.
cipher_data['Cipher_Words'] = cipher_data['Cipher_Words'].astype(str)
cipher_data['Cipher_Words'] = cipher_data['Cipher_Words'].str.replace(r'[^a-zA-Z \n\']', ' ')
cipher_data['Cipher_Words'] = cipher_data['Cipher_Words'].replace('\n', ' ')
cipher_data['Cipher_Words'] = cipher_data['Cipher_Words'].str.replace(r'\s+', ' ')
cipher_data['Cipher_Words'] = cipher_data['Cipher_Words'].str.replace(r'\s*\'\s*', "'")

# If stemming is needed, uncomment next 3 lines
# porter = PorterStemmer()
# cipher_words = cipher_data['Cipher_Words'].str.split().explode().unique()
# stemmed_cipher_words = [porter.stem(word) for word in cipher_words]

# use the cipher words as they are for the coOccur-Matrix x
cipher_words = cipher_data['Cipher_Words'].str.split().explode().unique()

# CountVectorizer with the cipher words as vocabulary
vectorizer_cipher = CountVectorizer(analyzer='word', vocabulary=cipher_words, tokenizer=lambda text: text.split())

# fit & transform the cipher words data to get coOccur-Matrix
X_cipher = vectorizer_cipher.fit_transform(cipher_data['Cipher_Words'])
co_occurrence_matrix_cipher = (X_cipher.T * X_cipher).toarray()

# Normalize coOccur-Matrix by the number of cipher blocks to get probabilities
co_occurrence_matrix_cipher_normalized = co_occurrence_matrix_cipher / X_cipher.shape[0]

# make Mc matrix as a DataFrame
Mc_matrix = pd.DataFrame(co_occurrence_matrix_cipher_normalized, index=cipher_words, columns=cipher_words)

# Save the Mc matrix to a CSV file
Mc_matrix.to_csv('Mc_matrix.csv', index=True)

