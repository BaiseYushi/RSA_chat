# RSA Chat

An educational peer-to-peer chat application demonstrating RSA encryption principles.  
Available for Windows (Qt/C++) and Android (Java/NDK).

**Version 1.1** | December 2025

![RSA Chat Logo](rsa_chat_256.png)

## Disclaimer

**For educational purposes only - not secure for real communication.**

This application uses simplified, intentionally weak cryptography to demonstrate RSA concepts:

- Key size is approximately 17 bits (real RSA uses 2048+ bits)
- No padding scheme (vulnerable to frequency analysis)  
- Keys transmitted in plaintext (no TLS)
- No authentication (vulnerable to man-in-the-middle attacks)

## Features

- RSA key pair generation
- Automatic key exchange between peers
- Encrypted messaging over TCP
- Cross-platform support (Windows and Android)
- Dark theme user interface
- **Preview mode** to view cipher length and encrypted data

## Requirements

> **Important:** Both devices must be on the **same WiFi network** (local network) to connect to each other. This app uses direct TCP connections and does not work over the internet without port forwarding.

## How It Works

1. Each user generates an RSA key pair (public and private key)
2. Users connect via TCP socket (one hosts, one connects)
3. Public keys are exchanged automatically
4. Messages are encrypted with the recipient's public key
5. Only the recipient can decrypt messages using their private key

## Building

### Windows (Qt/C++)

Requirements:
- Qt 6.8 or later (Widgets and Network modules)
- CMake 3.24 or later
- MSVC 2022 or compatible compiler

```
cd PC_Windows/rsa_chat
mkdir build
cd build
cmake ..
cmake --build .
```

### Android

Requirements:
- Android Studio
- NDK (for native RSA implementation)
- Minimum SDK: 24

Open `Android/RSA_chat` in Android Studio and build.

## Usage

1. **Connect to the same WiFi** - Both devices must be on the same local network
2. **Generate Keys** - Click the button to create your RSA key pair
3. **Share Your IP** - Give your IP address to your chat partner (only one person needs to connect)
4. **Connect** - Enter partner's IP and port (default: 12345)
5. **Chat** - Send encrypted messages!

**Tip:** Use the "Preview" toggle to view cipher length and encrypted data for educational purposes.

## Project Structure

```
RSA_chat/
    Android/              Android application (Java + NDK)
        RSA_chat/
    PC_Windows/           Windows application (Qt/C++)
        rsa_chat/
    master-logo.png       Application icon source
    rsa_chat.ico          Windows icon
    README.md
    LICENSE
```

## Author

Baiyu  
https://github.com/BaiseYushi

## License

CC BY-NC 4.0 - You may share and adapt this work for non-commercial purposes with attribution. See LICENSE file.
