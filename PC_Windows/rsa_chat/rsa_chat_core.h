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

std::string getLocalIP();

KeyPair generateAndSaveKeys(const std::string& myIP);

PublicKey loadPublicKey(const std::string& filename);

PrivateKey loadPrivateKey(const std::string& filename);

KeyPair generateKeys();

std::vector<int> encryptMessage(const std::string& message, const PublicKey& pub);

std::string decryptMessage(const std::vector<int>& cipher, const PrivateKey& priv);

void saveCipherToFile(const std::vector<int>& cipher, const std::string& filename);

std::vector<int> loadCipherFromFile(const std::string& filename);