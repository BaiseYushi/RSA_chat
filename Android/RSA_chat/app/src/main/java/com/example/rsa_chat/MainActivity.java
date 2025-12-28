package com.example.rsa_chat;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.text.Html;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("rsa_chat_core");
    }

    public native int[] nativeGenerateKeys();
    public native int[] nativeEncrypt(String msg, int e, int n);
    public native String nativeDecrypt(int[] cipher, int d, int n);
    public native String nativeGetLocalIP();
    public native void nativeSaveCipherToFile(int[] cipher, String filename);
    public native int[] nativeLoadCipherFromFile(String filename);

    private EditText serverIpEdit;
    private EditText serverPortEdit;
    private Button connectButton;
    private Button helpButton;
    private Button previewToggleButton;
    private TextView chatView;
    private EditText messageInput;
    private Button sendButton;
    private boolean previewEnabled = false;

    private ServerSocket serverSocket;
    private Socket socket;
    private BufferedReader reader;
    private PrintWriter writer;
    private Thread listenThread;
    private Thread serverThread;
    private volatile boolean connected = false;
    private volatile boolean serverRunning = false;

    private static final int SERVER_PORT = 12345;

    private int myE, myD, myN;
    private int peerE, peerN;
    private boolean keysExchanged = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        serverIpEdit = findViewById(R.id.server_ip);
        serverPortEdit = findViewById(R.id.server_port);
        connectButton = findViewById(R.id.connect_button);
        chatView = findViewById(R.id.chat_view);
        messageInput = findViewById(R.id.message_input);
        sendButton = findViewById(R.id.send_button);

        int[] keys = nativeGenerateKeys();
        myE = keys[0];
        myN = keys[1];
        myD = keys[2];

        String myIP = nativeGetLocalIP();

        appendToChat("Your IP: " + myIP);
        appendToChat("Listening on port: " + SERVER_PORT);
        appendToChat("RSA Keys generated!");
        appendToChat("Public (e,n): (" + myE + ", " + myN + ")");
        appendToChat("Private (d,n): (" + myD + ", " + myN + ")");
        appendToChat("\nWaiting for connection...\n");

        startServer();

        connectButton.setOnClickListener(v -> connectToServer());
        sendButton.setOnClickListener(v -> sendMessage());

        helpButton = findViewById(R.id.help_button);
        helpButton.setOnClickListener(v -> showHelp());

        previewToggleButton = findViewById(R.id.preview_toggle_button);
        previewToggleButton.setOnClickListener(v -> {
            previewEnabled = !previewEnabled;
            previewToggleButton.setText(previewEnabled ? "Preview: ON" : "Preview: OFF");
            Toast.makeText(this, "Preview " + (previewEnabled ? "enabled" : "disabled"), Toast.LENGTH_SHORT).show();
        });
    }

    private void showHelp() {
        String helpHtml = "<h2>RSA Chat - Help</h2>" +
                "<h3>About</h3>" +
                "<p><b>Author:</b> 白玉</p>" +
                "<p><b>Version:</b> 1.1</p>" +
                "<p><b>Date:</b> December 2025</p>" +
                "<p><b>Contact:</b> <a href=\"https://github.com/BaiseYushi\">GitHub</a></p>" +
                "<p>An educational peer-to-peer chat demonstrating RSA encryption principles.</p>" +
                "<h3>Disclaimer</h3>" +
                "<p><b>For educational purposes only - not secure for real communication.</b></p>" +
                "<p>This app uses simplified, intentionally weak cryptography to demonstrate RSA concepts.</p>" +
                "<h3>How to Use</h3>" +
                "<p>1. <b>Keys are generated automatically</b> when the app starts.</p>" +
                "<p>2. <b>Share Your IP:</b> Give your IP address to your friend. Only ONE person needs to do this.</p>" +
                "<p>3. <b>Connect:</b> Enter your friend's IP and port (default: 12345), then tap Connect.</p>" +
                "<p>4. <b>Chat:</b> Once connected, keys are exchanged automatically. Send encrypted messages!</p>" +
                "<h3>Why This Is Not Secure</h3>" +
                "<p>• Key size is tiny (~17 bits vs 2048+ in real RSA)</p>" +
                "<p>• No padding scheme (vulnerable to frequency analysis)</p>" +
                "<p>• Keys transmitted in plaintext (no TLS)</p>" +
                "<p>• No authentication (vulnerable to MITM attacks)</p>";

        TextView messageView = new TextView(this);
        messageView.setText(Html.fromHtml(helpHtml, Html.FROM_HTML_MODE_COMPACT));
        messageView.setMovementMethod(android.text.method.LinkMovementMethod.getInstance());
        messageView.setPadding(48, 24, 48, 24);
        messageView.setTextColor(getResources().getColor(R.color.text_primary, getTheme()));
        messageView.setLinkTextColor(getResources().getColor(R.color.dark_primary, getTheme()));

        new AlertDialog.Builder(this, R.style.DarkAlertDialog)
                .setTitle("Help")
                .setView(messageView)
                .setPositiveButton("OK", null)
                .show();
    }

    private void startServer() {
        serverThread = new Thread(() -> {
            try {
                serverSocket = new ServerSocket(SERVER_PORT);
                serverRunning = true;
                runOnUiThread(() -> appendToChat("Server started on port " + SERVER_PORT));

                while (serverRunning) {
                    try {
                        Socket clientSocket = serverSocket.accept();

                        runOnUiThread(() -> appendToChat("Incoming connection from " +
                                clientSocket.getInetAddress().getHostAddress()));

                        if (connected && socket != null) {
                            clientSocket.close();
                            runOnUiThread(() -> appendToChat("Rejected: already connected"));
                            continue;
                        }

                        setupConnection(clientSocket);

                    } catch (IOException e) {
                        if (serverRunning) {
                            e.printStackTrace();
                        }
                    }
                }
            } catch (IOException e) {
                e.printStackTrace();
                runOnUiThread(() -> {
                    appendToChat("Server failed to start: " + e.getMessage());
                    Toast.makeText(this, "Server error: " + e.getMessage(), Toast.LENGTH_LONG).show();
                });
            }
        });
        serverThread.start();
    }

    private void setupConnection(Socket newSocket) {
        if (socket != null) {
            try {
                socket.close();
            } catch (IOException ignored) {}
        }

        socket = newSocket;

        try {
            reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            writer = new PrintWriter(socket.getOutputStream(), true);
            connected = true;
            keysExchanged = false;

            runOnUiThread(() -> {
                Toast.makeText(this, "Connected!", Toast.LENGTH_SHORT).show();
                appendToChat("Connected! Exchanging keys...");
            });

            writer.println("KEY:" + myE + ":" + myN);
            writer.flush();

            listenThread = new Thread(this::listenLoop);
            listenThread.start();

        } catch (IOException e) {
            e.printStackTrace();
            runOnUiThread(() -> appendToChat("Connection setup failed: " + e.getMessage()));
            connected = false;
        }
    }

    private void connectToServer() {
        final String host = serverIpEdit.getText().toString().trim();
        final String portStr = serverPortEdit.getText().toString().trim();

        if (host.isEmpty()) {
            Toast.makeText(this, "Enter server IP", Toast.LENGTH_SHORT).show();
            return;
        }

        int port = SERVER_PORT;
        if (!portStr.isEmpty()) {
            try {
                port = Integer.parseInt(portStr);
            } catch (NumberFormatException e) {
                Toast.makeText(this, "Invalid port", Toast.LENGTH_SHORT).show();
                return;
            }
        }

        if (connected) {
            Toast.makeText(this, "Already connected", Toast.LENGTH_SHORT).show();
            return;
        }

        final int finalPort = port;
        appendToChat("Connecting to " + host + ":" + port + "...");

        new Thread(() -> {
            try {
                Socket newSocket = new Socket(host, finalPort);
                // setupConnection must run on background thread since it does socket I/O
                setupConnection(newSocket);

            } catch (IOException e) {
                e.printStackTrace();
                runOnUiThread(() -> {
                    Toast.makeText(this, "Connect failed: " + e.getMessage(), Toast.LENGTH_LONG).show();
                    appendToChat("Connect failed: " + e.getMessage());
                });
            }
        }).start();
    }

    private void listenLoop() {
        try {
            String line;
            while (connected && (line = reader.readLine()) != null) {
                final String msg = line;
                runOnUiThread(() -> handleIncomingMessage(msg));
            }
        } catch (IOException e) {
            if (connected) {
                e.printStackTrace();
                runOnUiThread(() -> appendToChat("Connection lost: " + e.getMessage()));
            }
        } finally {
            connected = false;
            keysExchanged = false;
            closeSocket();
        }
    }

    private void handleIncomingMessage(String line) {
        if (line.startsWith("KEY:")) {
            String[] parts = line.split(":");
            if (parts.length == 3) {
                try {
                    peerE = Integer.parseInt(parts[1]);
                    peerN = Integer.parseInt(parts[2]);

                    if (peerE <= 0 || peerN <= 0) {
                        appendToChat("Error: Peer sent invalid keys (negative values)");
                        appendToChat("Tell peer to click 'Generate Keys' first!");
                        return;
                    }

                    keysExchanged = true;
                    appendToChat("Keys exchanged! Peer's public key: (" + peerE + ", " + peerN + ")");
                    appendToChat("You can now chat securely!\n");
                } catch (NumberFormatException e) {
                    appendToChat("Error parsing peer's key");
                }
            }
        } else if (line.startsWith("MSG:")) {
            if (!keysExchanged) {
                appendToChat("Error: Received message before key exchange");
                return;
            }

            String cipherStr = line.substring(4);
            String[] parts = cipherStr.split(",");
            int[] cipher = new int[parts.length];

            try {
                for (int i = 0; i < parts.length; i++) {
                    cipher[i] = Integer.parseInt(parts[i].trim());
                }

                String decrypted = nativeDecrypt(cipher, myD, myN);

                if (previewEnabled) {
                    String cipherFile = getExternalFilesDir(null) + "/cipher_received.txt";
                    nativeSaveCipherToFile(cipher, cipherFile);
                    appendToChat("[Saved cipher to: " + cipherFile + "]");

                    String plainFile = getExternalFilesDir(null) + "/plain_received.txt";
                    try {
                        java.io.FileWriter fw = new java.io.FileWriter(plainFile);
                        fw.write(decrypted);
                        fw.close();
                        appendToChat("[Saved plaintext to: " + plainFile + "]");
                    } catch (Exception e) {
                        appendToChat("[Error saving plaintext: " + e.getMessage() + "]");
                    }
                }

                appendToChat("Peer: " + decrypted);
            } catch (NumberFormatException e) {
                appendToChat("Error decrypting message");
            }
        }
    }

    private void sendMessage() {
        if (!connected || writer == null) {
            Toast.makeText(this, "Not connected", Toast.LENGTH_SHORT).show();
            return;
        }

        if (!keysExchanged) {
            Toast.makeText(this, "Keys not exchanged yet", Toast.LENGTH_SHORT).show();
            return;
        }

        String text = messageInput.getText().toString().trim();
        if (text.isEmpty()) return;

//        appendToChat("Attempting to encrypt with e=" + peerE + " n=" + peerN);

        try {
//            int[] testCipher = nativeEncrypt("A", peerE, peerN);
//            if (testCipher == null || testCipher.length == 0) {
//                appendToChat("TEST FAILED: Can't even encrypt 'A'!");
//                throw new Exception("Test encryption failed");
//            }
//            appendToChat("TEST OK: Encrypted 'A' -> " + testCipher[0]);

            int[] cipher = nativeEncrypt(text, peerE, peerN);

            if (cipher == null || cipher.length == 0) {
                throw new Exception("Encryption returned empty result");
            }

            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < cipher.length; i++) {
                sb.append(cipher[i]);
                if (i < cipher.length - 1) sb.append(",");
            }

            if (previewEnabled) {
                appendToChat("[Length: " + cipher.length + "]");
                appendToChat("[Cipher: " + sb.toString() + "]");
                try {
                    String cipherFile = getExternalFilesDir(null) + "/cipher_sent.txt";
                    nativeSaveCipherToFile(cipher, cipherFile);
                    appendToChat("[Saved to: " + cipherFile + "]");
                } catch (Exception e) {
                    appendToChat("[File save error: " + e.getMessage() + "]");
                }
            }

            String msgToSend = "MSG:" + sb.toString();
//            appendToChat("Sending: " + msgToSend);

            new Thread(() -> {
                try {
                    writer.println(msgToSend);
                    writer.flush();

//                    runOnUiThread(() -> appendToChat("Socket write complete!"));
                } catch (Exception ex) {
                    String errMsg = ex.getMessage() != null ? ex.getMessage() : "Unknown send error";
                    String stackTrace = android.util.Log.getStackTraceString(ex);

                    runOnUiThread(() -> {
                        Toast.makeText(MainActivity.this, "Send error: " + errMsg, Toast.LENGTH_LONG).show();
                        appendToChat("ERROR sending: " + errMsg);
                        appendToChat("Stack: " + stackTrace);
                    });
                }
            }).start();

            appendToChat("Me: " + text);
            messageInput.setText("");
        } catch (Exception e) {
            e.printStackTrace();
            String errMsg = e.getMessage() != null ? e.getMessage() : "Unknown error";
            String stackTrace = android.util.Log.getStackTraceString(e);
            Toast.makeText(this, "Error: " + errMsg, Toast.LENGTH_LONG).show();
            appendToChat("ERROR: " + errMsg);
            appendToChat("Stack: " + stackTrace);
        }
    }

    private void appendToChat(String line) {
        String current = chatView.getText().toString();
        if (current.isEmpty() || current.equals("Not connected.")) {
            chatView.setText(line);
        } else {
            chatView.setText(current + "\n" + line);
        }
    }

    private void closeSocket() {
        try {
            if (reader != null) reader.close();
        } catch (IOException ignored) {}
        if (writer != null) writer.close();
        try {
            if (socket != null) socket.close();
        } catch (IOException ignored) {}
        reader = null;
        writer = null;
        socket = null;
    }

    @Override
    protected void onDestroy() {
        connected = false;
        serverRunning = false;

        closeSocket();

        try {
            if (serverSocket != null) serverSocket.close();
        } catch (IOException ignored) {}

        super.onDestroy();
    }
}