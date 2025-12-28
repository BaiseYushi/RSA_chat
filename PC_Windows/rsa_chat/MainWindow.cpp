#include "MainWindow.h"
#include "ChatPage.h"
#include "SetupPage.h"
#include "rsa_chat_core.h"
#include <QDebug>
#include <QFile>
#include <QHostAddress>
#include <QKeyEvent>
#include <QMessageBox>
#include <QStackedWidget>
#include <QTcpServer>
#include <QTimer>
#include <fstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_stack(new QStackedWidget(this)),
      m_setupPage(new SetupPage(this)), m_chatPage(new ChatPage(this)),
      m_server(new QTcpServer(this)), m_socket(nullptr) {
  setupUi();
  setupConnections();

  // Step 1: Get and display local IP
  m_myIP = QString::fromStdString(getLocalIP());
  m_setupPage->setStatusText("Your IP: " + m_myIP +
                             "\nListening on port 12345");

  startServer(12345);
}

MainWindow::~MainWindow() {
  deleteKeyFiles();
  if (m_socket) {
    m_socket->disconnectFromHost();
    m_socket->deleteLater();
  }
}

void MainWindow::deleteKeyFiles() {
  QString ipClean = m_myIP;
  ipClean.replace('.', '_');

  QString pubFile = ipClean + "_public.key";
  QString privFile = ipClean + "_private.key";

  if (QFile::exists(pubFile)) {
    QFile::remove(pubFile);
    qInfo() << "Deleted:" << pubFile;
  }
  if (QFile::exists(privFile)) {
    QFile::remove(privFile);
    qInfo() << "Deleted:" << privFile;
  }

  // Also delete peer's public key file if we have a connection
  if (m_socket) {
    QString peerIP = m_socket->peerAddress().toString();
    if (peerIP.startsWith("::ffff:")) {
      peerIP = peerIP.mid(7);
    }
    QString peerClean = peerIP;
    peerClean.replace('.', '_');
    peerClean.replace(':', '_');
    QString peerPubFile = peerClean + "_public.key";
    if (QFile::exists(peerPubFile)) {
      QFile::remove(peerPubFile);
      qInfo() << "Deleted:" << peerPubFile;
    }
  }
}

void MainWindow::setupUi() {
  m_stack->addWidget(m_setupPage);
  m_stack->addWidget(m_chatPage);
  m_stack->setCurrentWidget(m_setupPage);

  setCentralWidget(m_stack);
  setWindowTitle("RSA Chat (Press F5 for Help)");
  resize(600, 400);
}

void MainWindow::setupConnections() {
  connect(m_setupPage, &SetupPage::generateKeysRequested, this,
          &MainWindow::handleGenerateKeys);
  connect(m_setupPage, &SetupPage::connectToServer, this,
          &MainWindow::handleConnectToServer);

  connect(m_chatPage, &ChatPage::sendMessageRequested, this,
          &MainWindow::handleSendMessageRequested);

  connect(m_server, &QTcpServer::newConnection, this,
          &MainWindow::handleNewIncomingConnection);
}

void MainWindow::startServer(quint16 port) {
  if (!m_server->listen(QHostAddress::Any, port)) {
    qWarning() << "Server listen failed:" << m_server->errorString();
    m_setupPage->setStatusText("Error: " + m_server->errorString());
  } else {
    qInfo() << "Server listening on port" << m_server->serverPort();
  }
}

void MainWindow::setupSocket(QTcpSocket *socket) {
  connect(socket, &QTcpSocket::connected, this,
          &MainWindow::handleSocketConnected);
  connect(socket, &QTcpSocket::readyRead, this,
          &MainWindow::handleSocketReadyRead);
  connect(socket, &QTcpSocket::errorOccurred, this,
          &MainWindow::handleSocketError);
  connect(socket, &QTcpSocket::disconnected, this,
          &MainWindow::handleSocketDisconnected);
}

void MainWindow::handleGenerateKeys() {
  // Step 2: Generate keys with IP in filename
  m_keys = generateAndSaveKeys(m_myIP.toStdString());

  QString ipClean = m_myIP;
  ipClean.replace('.', '_');

  QString info =
      QString(
          "Keys generated!\n%1_public.key\n%1_private.key\n\nn=%2, e=%3, d=%4")
          .arg(ipClean)
          .arg(m_keys.pub.n)
          .arg(m_keys.pub.e)
          .arg(m_keys.priv.d);
  m_setupPage->appendStatusText(info);
}

void MainWindow::handleConnectToServer(const QString &host, quint16 port) {
  if (m_socket) {
    m_socket->deleteLater();
    m_socket = nullptr;
  }

  auto *socket = new QTcpSocket(this);
  m_socket = socket;
  setupSocket(socket);

  m_setupPage->setStatusText("Connecting to " + host + ":" +
                             QString::number(port) + "...");
  socket->connectToHost(host, port);
}

void MainWindow::handleNewIncomingConnection() {
  QTcpSocket *client = m_server->nextPendingConnection();
  if (!client)
    return;

  if (m_socket) {
    m_socket->deleteLater();
  }

  m_socket = client;
  setupSocket(client);

  QString peerIP = client->peerAddress().toString();
  m_setupPage->setStatusText("Peer connected from " + peerIP +
                             "\nExchanging keys...");

  // Send our public key immediately
  sendPublicKey();
}

void MainWindow::handleSocketConnected() {
  m_setupPage->setStatusText("Connected! Exchanging keys...");
  // Send our public key immediately
  sendPublicKey();
}

void MainWindow::sendPublicKey() {
  if (!m_socket)
    return;

  QString msg = QString("KEY:%1:%2\n").arg(m_keys.pub.e).arg(m_keys.pub.n);
  m_socket->write(msg.toUtf8());
  m_socket->flush();

  qInfo() << "Sent public key:" << m_keys.pub.e << m_keys.pub.n;
}

void MainWindow::handleSocketReadyRead() {
  if (!m_socket)
    return;

  QByteArray data = m_socket->readAll();
  QString message = QString::fromUtf8(data).trimmed();

  QStringList lines = message.split('\n', Qt::SkipEmptyParts);
  for (const QString &line : lines) {
    if (line.startsWith("KEY:")) {
      // Received their public key
      QStringList parts = line.split(':');
      if (parts.size() == 3) {
        bool ok1, ok2;
        int e = parts[1].toInt(&ok1);
        int n = parts[2].toInt(&ok2);
        if (ok1 && ok2) {
          m_remotePublicKey.e = e;
          m_remotePublicKey.n = n;

          // Save their key to file
          QString peerIP = m_socket->peerAddress().toString();
          if (peerIP.startsWith("::ffff:")) {
            peerIP = peerIP.mid(7);
          }
          QString ipClean = peerIP;
          ipClean.replace('.', '_');
          ipClean.replace(':', '_');

          std::string filename = ipClean.toStdString() + "_public.key";
          std::ofstream file(filename);
          file << e << " " << n;
          file.close();

          qInfo() << "Received public key - e:" << e << "n:" << n;
          m_chatPage->appendMessage("System",
                                    "Keys exchanged! You can now chat.");
          m_stack->setCurrentWidget(m_chatPage);
        }
      }
    } else if (line.startsWith("MSG:")) {
      // Received encrypted message
      QString cipherStr = line.mid(4); // Remove "MSG:"
      QStringList cipherParts = cipherStr.split(',', Qt::SkipEmptyParts);

      std::vector<int> cipher;
      for (const QString &part : cipherParts) {
        bool ok;
        int val = part.toInt(&ok);
        if (ok)
          cipher.push_back(val);
      }

      if (!cipher.empty()) {
        std::string plain = decryptMessage(cipher, m_keys.priv);
        m_chatPage->appendMessage("Peer", QString::fromStdString(plain));
      }
    }
  }
}

void MainWindow::handleSocketError(QAbstractSocket::SocketError socketError) {
  Q_UNUSED(socketError);
  if (m_socket) {
    m_chatPage->appendMessage("System", "Error: " + m_socket->errorString());
  }
}

void MainWindow::handleSocketDisconnected() {
  deleteKeyFiles();
  m_chatPage->appendMessage("System", "Peer disconnected. Keys deleted.");
}

void MainWindow::handleSendMessageRequested(const QString &text) {
  if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
    m_chatPage->appendMessage("System", "Not connected!");
    return;
  }

  // Encrypt message with THEIR public key
  std::string plain = text.toStdString();
  std::vector<int> cipher = encryptMessage(plain, m_remotePublicKey);

  // Convert cipher to comma-separated string
  QString cipherStr;
  for (size_t i = 0; i < cipher.size(); ++i) {
    cipherStr += QString::number(cipher[i]);
    if (i < cipher.size() - 1)
      cipherStr += ",";
  }

  // Show preview info if enabled
  if (m_chatPage->isPreviewEnabled()) {
    m_chatPage->appendPreviewInfo(QString("[Length: %1]").arg(cipher.size()));
    m_chatPage->appendPreviewInfo(QString("[Cipher: %1]").arg(cipherStr));
  }

  // Send encrypted message over socket
  m_socket->write(("MSG:" + cipherStr + "\n").toUtf8());
  m_socket->flush();

  // Show in own chat
  m_chatPage->appendMessage("Me", text);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_F5) {
    showHelp();
  } else {
    QMainWindow::keyPressEvent(event);
  }
}

void MainWindow::showHelp() {
  QString helpText = R"(
<h2>RSA Chat - Help</h2>

<h3>About</h3>
<p><b>Author:</b> 白玉</p>
<p><b>Version:</b> 1.1</p>
<p><b>Date:</b> December 2025</p>
<p><b>Contact:</b> <a href="https://github.com/BaiseYushi">GitHub</a></p>
<p>An educational peer-to-peer chat demonstrating RSA encryption principles.</p>

<h3>Disclaimer</h3>
<p><b>For educational purposes only - not secure for real communication.</b></p>
<p>This app uses simplified, intentionally weak cryptography to demonstrate RSA concepts.</p>

<h3>How to Use</h3>
<ol>
<li><b>Generate Keys:</b> Click "Generate Keys" to create your RSA key pair.</li>
<li><b>Share Your IP:</b> Give your IP address to your friend. Only ONE person needs to do this.</li>
<li><b>Connect:</b> Enter your friend's IP and port (default: 12345), then click "Connect".</li>
<li><b>Chat:</b> Once connected, keys are exchanged automatically. Send encrypted messages!</li>
</ol>

<h3>Why This Is Not Secure</h3>
<ul>
<li>Key size is tiny (~17 bits vs 2048+ bits in real RSA)</li>
<li>No padding scheme (vulnerable to frequency analysis)</li>
<li>Keys transmitted in plaintext (no TLS)</li>
<li>No authentication (vulnerable to MITM attacks)</li>
</ul>

<h3>Keyboard Shortcuts</h3>
<ul>
<li><b>F5</b> - Show this help</li>
</ul>
)";

  QMessageBox helpBox(this);
  helpBox.setWindowTitle("RSA Chat - Help");
  helpBox.setTextFormat(Qt::RichText);
  helpBox.setText(helpText);
  helpBox.setIcon(QMessageBox::Information);
  helpBox.exec();
}