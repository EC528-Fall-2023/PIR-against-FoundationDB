#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/aes.h>
#include <string.h>
#include <fstream>

void handleErrors() {
    ERR_print_errors_fp(stderr);
    abort();
}

std::string to_hex(const std::string& input) {
    std::stringstream hex_stream;
    hex_stream << std::hex << std::setfill('0');
    for (unsigned char c : input) {
        hex_stream << std::setw(2) << static_cast<int>(c);
    }
    return hex_stream.str();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
            unsigned char *iv, unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    //EVP_CIPHER_CTX_set_padding(ctx, 0); // Disable padding

    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
            unsigned char *iv, unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;

    if (!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        handleErrors();

    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) handleErrors();
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

int main() {
    // Load the necessary cipher
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();

    // Hardcoded keys for demonstration purposes
    unsigned char key[] = "0123456789abcdeF0123456789abcdeF"; // 32 bytes for AES256
    unsigned char iv[] = "1234567890abcdef"; // 16 bytes for AES block size

    // Open the input CSV file
    std::ifstream inputFile("spam_ham_dataset.csv");
    // Open the output CSV file
    std::ofstream outputFile("encrypted_spam_ham_dataset.csv");

    std::string line;
    bool isFirstRow = true;

    while (std::getline(inputFile, line)) {
        if (isFirstRow) {
            // Write the header (first row) directly to the output file
            outputFile << line << std::endl;
            isFirstRow = false;
        } else {
            // Process the subsequent rows with encryption
            std::stringstream lineStream(line);
            std::string cell;
            bool firstCell = true;

            while (std::getline(lineStream, cell, ',')) {
                unsigned char ciphertext[128];
                int ciphertext_len;

                // Encrypt the cell content
                ciphertext_len = encrypt((unsigned char*)cell.c_str(), cell.length(), key, iv, ciphertext);

                std::ostringstream processed_ciphertext;
                for (int i = 0; i < ciphertext_len; ++i) {
                    char val = static_cast<char>(ciphertext[i]);
                    if (std::isprint(static_cast<unsigned char>(val)) || std::isspace(static_cast<unsigned char>(val))) {
                        processed_ciphertext << val;
                    } else {
                        processed_ciphertext << '.'; // Replace non-printable characters with a dot
                    }
                }

                if (!firstCell) {
                    outputFile << ",";
                }
                outputFile << processed_ciphertext.str(); // Write processed ciphertext to the output file
                firstCell = false;
            }
            outputFile << std::endl;
        }
    }

    inputFile.close();
    outputFile.close();

    // Clean up
    EVP_cleanup();
    ERR_free_strings();

    return 0;
}