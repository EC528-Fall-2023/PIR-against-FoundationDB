import csv

def extract_columns(file_path, key_column, value_column, max_value_length):
    with open(file_path, 'r') as file:
        reader = csv.DictReader(file)
        with open('keys.txt', 'w') as keys_file, open('values.txt', 'w') as values_file:
            for row in reader:
                key = row[key_column]
                value = row[value_column]

                # Truncate the value if it exceeds the maximum length
                if len(value) > max_value_length:
                    value = value[:max_value_length]

                keys_file.write(key + '\n')
                values_file.write(value.replace('\n', ' ') + '\n')

if __name__ == "__main__":
    csv_file_path = 'spam_ham_dataset.csv'
    key_column_name = 'id'
    value_column_name = 'text'
    max_value_length = 1022

    extract_columns(csv_file_path, key_column_name, value_column_name, max_value_length)

