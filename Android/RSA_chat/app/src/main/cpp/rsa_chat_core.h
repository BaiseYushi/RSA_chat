#pragma once

#include <string>
#include <vector>

struct PublicKey {
    int e;
    int n;
};

struct PrivateKey {
    int d;
    int n;
};

struct KeyPair {
    PublicKey pub;
    PrivateKey priv;
};

// Get local IP address
std::string getLocalIP();

// Generate keypair and save to files with IP prefix
KeyPair generateAndSaveKeys(const std::string& myIP);

// Load public key from file
PublicKey loadPublicKey(const std::string& filename);

// Load private key from file
PrivateKey loadPrivateKey(const std::string& filename);

// Generate a fresh keypair (internal use)
KeyPair generateKeys();

// Encrypt message
std::vector<int> encryptMessage(const std::string& message, const PublicKey& pub);

// Decrypt message
std::string decryptMessage(const std::vector<int>& cipher, const PrivateKey& priv);

// Save cipher to file
void saveCipherToFile(const std::vector<int>& cipher, const std::string& filename);

// Load cipher from file
std::vector<int> loadCipherFromFile(const std::string& filename);