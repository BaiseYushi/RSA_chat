#pragma once

#include "rsa_chat_core.h"
#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

class QStackedWidget;
class SetupPage;
class ChatPage;
class QKeyEvent;

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

protected:
  void keyPressEvent(QKeyEvent *event) override;

private slots:
  void handleGenerateKeys();
  void handleConnectToServer(const QString &host, quint16 port);
  void handleNewIncomingConnection();
  void handleSocketConnected();
  void handleSocketReadyRead();
  void handleSocketError(QAbstractSocket::SocketError socketError);
  void handleSocketDisconnected();
  void handleSendMessageRequested(const QString &text);

private:
  void setupUi();
  void setupConnections();
  void setupSocket(QTcpSocket *socket);
  void startServer(quint16 port);
  void sendPublicKey();
  void deleteKeyFiles();
  void showHelp();

  QStackedWidget *m_stack;
  SetupPage *m_setupPage;
  ChatPage *m_chatPage;

  QTcpServer *m_server;
  QTcpSocket *m_socket;

  QString m_myIP;
  KeyPair m_keys;
  PublicKey m_remotePublicKey;
};