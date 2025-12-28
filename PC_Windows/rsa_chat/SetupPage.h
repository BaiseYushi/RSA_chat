//
// Created by Baiyu on 01/12/2025.
//

#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QLabel;

class SetupPage : public QWidget {
  Q_OBJECT
public:
  explicit SetupPage(QWidget *parent = nullptr);

  void setStatusText(const QString &text);
  void appendStatusText(const QString &text);
  void setContinueEnabled(bool enabled);

signals:
  void generateKeysRequested();
  void connectToServer(const QString &host, quint16 port);
  void continueToChatRequested();

private slots:
  void onGenerateKeysButtonClicked();
  void onConnectButtonClicked();

private:
  QLineEdit *m_hostEdit;
  QLineEdit *m_portEdit;
  QPushButton *m_generateKeysButton;
  QPushButton *m_connectButton;
  QPushButton *m_continueButton;
  QLabel *m_statusLabel;
};
