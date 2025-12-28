#include "rsa_chat_core.h"
#include <random>
#include <fstream>
#include <sstream>
#include <QNetworkInterface>
#include <QHostAddress>

static bool is_prime(int n) {
    if (n <= 1) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0) return false;
    for (int i = 3; i * i <= n; i += 2)
        if (n % i == 0) return false;
    return true;
}

static int generate_prime(std::mt19937& gen) {
    std::uniform_int_distribution<> dist(100, 500);
    while (true) {
        int p = dist(gen);
        if (is_prime(p)) return p;
    }
}

static int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

static int modinv(int a, int m) {
    int m0 = m, t, q, x0 = 0, x1 = 1;
    while (a > 1) {
        q = a / m;
        t = m;
        m = a % m;
        a = t;
        t = x0;
        x0 = x1 - q * x0;
        x1 = t;
    }
    return x1 < 0 ? x1 + m0 : x1;
}

static int modpow(int base, int exp, int mod) {
    long long result = 1;
    long long b = base % mod;
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = (result * b) % mod;
        }
        b = (b * b) % mod;
        exp /= 2;
    }
    return (int)result;
}

std::string getLocalIP() {
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

    for (const QHostAddress& addr : addresses) {
        if (!addr.isLoopback() && addr.protocol() == QAbstractSocket::IPv4Protocol) {
            QString ip = addr.toString();
            if (ip.startsWith("172.")) {
                QStringList parts = ip.split('.');
                if (parts.size() >= 2) {
                    int second = parts[1].toInt();
                    if (second >= 16 && second <= 31) {
                        return ip.toStdString();
                    }
                }
            }
        }
    }

    for (const QHostAddress& addr : addresses) {
        if (!addr.isLoopback() && addr.protocol() == QAbstractSocket::IPv4Protocol) {
            QString ip = addr.toString();
            if (ip.startsWith("172.20.")) {
                return ip.toStdString();
            }
        }
    }

    for (const QHostAddress& addr : addresses) {
        if (!addr.isLoopback() && addr.protocol() == QAbstractSocket::IPv4Protocol) {
            QString ip = addr.toString();
            if (ip.startsWith("192.168.")) {
                return ip.toStdString();
            }
        }
    }

    for (const QHostAddress& addr : addresses) {
        if (!addr.isLoopback() && addr.protocol() == QAbstractSocket::IPv4Protocol) {
            return addr.toString().toStdString();
        }
    }

    return "127.0.0.1";
}

KeyPair generateKeys() {
    std::random_device rd;
    std::mt19937 gen(rd());

    int p = generate_prime(gen);
    int q = generate_prime(gen);
    while (q == p) q = generate_prime(gen);

    int n = p * q;
    int phi = (p - 1) * (q - 1);

    int e = 3;
    while (gcd(e, phi) != 1) e += 2;

    int d = modinv(e, phi);

    KeyPair kp;
    kp.pub = PublicKey{e, n};
    kp.priv = PrivateKey{d, n};
    return kp;
}

KeyPair generateAndSaveKeys(const std::string& myIP) {
    KeyPair kp = generateKeys();

    std::string ipForFile = myIP;
    for (char& c : ipForFile) {
        if (c == '.') c = '_';
    }

    std::string pubFile = ipForFile + "_public.key";
    std::string privFile = ipForFile + "_private.key";

    std::ofstream pub(pubFile);
    pub << kp.pub.e << " " << kp.pub.n;
    pub.close();

    std::ofstream priv(privFile);
    priv << kp.priv.d << " " << kp.priv.n;
    priv.close();

    return kp;
}

PublicKey loadPublicKey(const std::string& filename) {
    std::ifstream file(filename);
    PublicKey key;
    file >> key.e >> key.n;
    file.close();
    return key;
}

PrivateKey loadPrivateKey(const std::string& filename) {
    std::ifstream file(filename);
    PrivateKey key;
    file >> key.d >> key.n;
    file.close();
    return key;
}

std::vector<int> encryptMessage(const std::string& message, const PublicKey& pub) {
    std::vector<int> cipher;
    cipher.reserve(message.size());
    for (unsigned char c : message) {
        long long ciph = modpow(static_cast<int>(c), pub.e, pub.n);
        cipher.push_back(static_cast<int>(ciph));
    }
    return cipher;
}

std::string decryptMessage(const std::vector<int>& cipher, const PrivateKey& priv) {
    std::string msg;
    msg.reserve(cipher.size());
    for (int num : cipher) {
        long long plain = modpow(num, priv.d, priv.n);
        msg.push_back(static_cast<char>(plain));
    }
    return msg;
}

void saveCipherToFile(const std::vector<int>& cipher, const std::string& filename) {
    std::ofstream file(filename);
    for (size_t i = 0; i < cipher.size(); ++i) {
        file << cipher[i];
        if (i < cipher.size() - 1) file << " ";
    }
    file.close();
}

std::vector<int> loadCipherFromFile(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<int> cipher;
    int num;
    while (file >> num) {
        cipher.push_back(num);
    }
    file.close();
    return cipher;
}