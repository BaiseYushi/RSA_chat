#include "rsa_chat_core.h"

#include <jni.h>
#include <random>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <android/log.h>

#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#define LOG_TAG "RSA_NATIVE"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ---------- RSA helpers---------

static bool is_prime(int n) {
    if (n <= 1) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0) return false;
    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
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
    if (mod == 1) return 0;

    long long result = 1;
    long long b = static_cast<long long>(base) % mod;
    if (b < 0) b += mod;

    while (exp > 0) {
        if (exp & 1) {
            result = (result * b) % mod;
        }
        b = (b * b) % mod;
        exp >>= 1;
    }
    return static_cast<int>(result);
}

// ---------- IP helper ----------

std::string getLocalIP() {
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) {
        return "127.0.0.1";
    }

    std::string chosen = "127.0.0.1";

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;

        auto* sa = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
        char buf[INET_ADDRSTRLEN] = {0};
        const char* res = inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf));
        if (!res) continue;

        std::string ip = buf;
        if (ip == "127.0.0.1") continue;

        if (ip.rfind("172.20.", 0) == 0) {
            chosen = ip;
            break;
        }
        if (ip.rfind("192.168.", 0) == 0) {
            chosen = ip;
        }
        if (chosen == "127.0.0.1") {
            chosen = ip;
        }
    }

    freeifaddrs(ifaddr);
    return chosen;
}

// ----------RSA implementation--------

KeyPair generateKeys() {
    std::random_device rd;
    std::mt19937 gen(rd());

    int p = generate_prime(gen);
    int q = generate_prime(gen);
    while (q == p) q = generate_prime(gen);

    int n   = p * q;
    int phi = (p - 1) * (q - 1);

    int e = 3;
    while (gcd(e, phi) != 1) e += 2;

    int d = modinv(e, phi);

    KeyPair kp;
    kp.pub  = PublicKey{e, n};
    kp.priv = PrivateKey{d, n};
    LOGI("Generated RSA keys: e=%d, n=%d, d=%d", e, n, d);
    return kp;
}

KeyPair generateAndSaveKeys(const std::string& myIP) {
    KeyPair kp = generateKeys();

    std::string ipForFile = myIP;
    for (char& c : ipForFile) {
        if (c == '.') c = '_';
    }

    std::string pubFile  = ipForFile + "_public.key";
    std::string privFile = ipForFile + "_private.key";

    std::ofstream pub(pubFile);
    pub << kp.pub.e << " " << kp.pub.n;
    pub.close();

    std::ofstream priv(privFile);
    priv << kp.priv.d << " " << kp.priv.n;
    priv.close();

    LOGI("Saved keys to %s / %s", pubFile.c_str(), privFile.c_str());
    return kp;
}

PublicKey loadPublicKey(const std::string& filename) {
    std::ifstream file(filename);
    PublicKey key{};
    file >> key.e >> key.n;
    file.close();
    return key;
}

PrivateKey loadPrivateKey(const std::string& filename) {
    std::ifstream file(filename);
    PrivateKey key{};
    file >> key.d >> key.n;
    file.close();
    return key;
}

std::vector<int> encryptMessage(const std::string& message, const PublicKey& pub) {
    std::vector<int> cipher;
    cipher.reserve(message.size());

    for (unsigned char c : message) {
        int ciph = modpow(static_cast<int>(c), pub.e, pub.n);
        cipher.push_back(ciph);
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
        if (i + 1 < cipher.size()) file << " ";
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

// ---------- JNI methods ----------

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_rsa_1chat_MainActivity_nativeGetLocalIP(
        JNIEnv* env,
        jobject /*thiz*/) {

    std::string ip = getLocalIP();
    return env->NewStringUTF(ip.c_str());
}

extern "C"
JNIEXPORT jintArray JNICALL
Java_com_example_rsa_1chat_MainActivity_nativeGenerateKeys(
        JNIEnv* env,
        jobject /*thiz*/) {

    KeyPair kp = generateKeys();

    jintArray result = env->NewIntArray(3);
    if (!result) {
        LOGE("Failed to allocate jintArray for keys");
        return nullptr;
    }

    jint values[3];
    values[0] = kp.pub.e;
    values[1] = kp.pub.n;
    values[2] = kp.priv.d;

    env->SetIntArrayRegion(result, 0, 3, values);
    return result;
}

extern "C"
JNIEXPORT jintArray JNICALL
Java_com_example_rsa_1chat_MainActivity_nativeEncrypt(
        JNIEnv* env,
        jobject /*thiz*/,
        jstring msg,
        jint e,
        jint n) {

    LOGI("nativeEncrypt called: e=%d, n=%d", e, n);

    if (n <= 0 || e <= 0) {
        LOGE("Invalid keys: e=%d, n=%d", e, n);
        return env->NewIntArray(0);
    }

    const char* utf = env->GetStringUTFChars(msg, nullptr);
    if (!utf) {
        LOGE("Failed to get UTF chars from Java string");
        return env->NewIntArray(0);
    }

    std::string str(utf);
    env->ReleaseStringUTFChars(msg, utf);

    LOGI("Message to encrypt: '%s' (len=%zu)", str.c_str(), str.size());

    PublicKey pub{e, n};
    std::vector<int> cipher;

    try {
        cipher = encryptMessage(str, pub);
    } catch (const std::exception& ex) {
        LOGE("Encryption exception: %s", ex.what());
        return env->NewIntArray(0);
    } catch (...) {
        LOGE("Unknown encryption error");
        return env->NewIntArray(0);
    }

    if (cipher.empty()) {
        LOGE("Cipher is empty after encryption");
        return env->NewIntArray(0);
    }

    jintArray result = env->NewIntArray(static_cast<jsize>(cipher.size()));
    if (!result) {
        LOGE("Failed to allocate jintArray for cipher");
        return env->NewIntArray(0);
    }

    env->SetIntArrayRegion(result, 0,
                           static_cast<jsize>(cipher.size()),
                           cipher.data());
    LOGI("Returning cipher of length %zu", cipher.size());
    return result;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_rsa_1chat_MainActivity_nativeDecrypt(
        JNIEnv* env,
        jobject /*thiz*/,
        jintArray cipherArray,
        jint d,
        jint n) {

    jsize len = env->GetArrayLength(cipherArray);
    if (len <= 0) {
        return env->NewStringUTF("");
    }

    jint* elements = env->GetIntArrayElements(cipherArray, nullptr);
    if (!elements) {
        return env->NewStringUTF("");
    }

    std::vector<int> cipher(elements, elements + len);
    env->ReleaseIntArrayElements(cipherArray, elements, JNI_ABORT);

    PrivateKey priv{d, n};
    std::string plain = decryptMessage(cipher, priv);

    return env->NewStringUTF(plain.c_str());
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_rsa_1chat_MainActivity_nativeSaveCipherToFile(
        JNIEnv* env,
        jobject /*thiz*/,
        jintArray cipherArray,
        jstring filename) {

    const char* fname = env->GetStringUTFChars(filename, nullptr);
    if (!fname) return;

    std::string filepath(fname);
    env->ReleaseStringUTFChars(filename, fname);

    jsize len = env->GetArrayLength(cipherArray);
    if (len <= 0) return;

    jint* elements = env->GetIntArrayElements(cipherArray, nullptr);
    if (!elements) return;

    std::vector<int> cipher(elements, elements + len);
    env->ReleaseIntArrayElements(cipherArray, elements, JNI_ABORT);

    saveCipherToFile(cipher, filepath);
    LOGI("Saved cipher to: %s", filepath.c_str());
}

extern "C"
JNIEXPORT jintArray JNICALL
Java_com_example_rsa_1chat_MainActivity_nativeLoadCipherFromFile(
        JNIEnv* env,
        jobject /*thiz*/,
        jstring filename) {

    const char* fname = env->GetStringUTFChars(filename, nullptr);
    if (!fname) {
        return env->NewIntArray(0);
    }

    std::string filepath(fname);
    env->ReleaseStringUTFChars(filename, fname);

    std::vector<int> cipher = loadCipherFromFile(filepath);
    LOGI("Loaded cipher from: %s (size=%zu)", filepath.c_str(), cipher.size());

    jintArray result = env->NewIntArray(static_cast<jsize>(cipher.size()));
    if (result && !cipher.empty()) {
        env->SetIntArrayRegion(result, 0,
                               static_cast<jsize>(cipher.size()),
                               cipher.data());
    }
    return result;
}
