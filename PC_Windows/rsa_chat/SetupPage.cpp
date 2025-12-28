//
// Created by Baiyu on 01/12/2025.
//

#include "SetupPage.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>


SetupPage::SetupPage(QWidget *parent)
    : QWidget(parent), m_hostEdit(new QLineEdit(this)),
      m_portEdit(new QLineEdit(this)),
      m_generateKeysButton(new QPushButton("Generate Keys", this)),
      m_connectButton(new QPushButton("Connect", this)),
      m_continueButton(new QPushButton("Go to Chat", this)),
      m_statusLabel(new QLabel(this)) {
  m_hostEdit->setPlaceholderText("Friend's IP (e.g. 192.168.x.x)");
  m_portEdit->setPlaceholderText("12345");

  auto *formLayout = new QFormLayout;
  formLayout->addRow("Server IP:", m_hostEdit);
  formLayout->addRow("Port:", m_portEdit);

  auto *buttonRow = new QHBoxLayout;
  buttonRow->addWidget(m_generateKeysButton);
  buttonRow->addWidget(m_connectButton);
  buttonRow->addWidget(m_continueButton);

  auto *mainLayout = new QVBoxLayout;
  mainLayout->addLayout(formLayout);
  mainLayout->addLayout(buttonRow);
  mainLayout->addWidget(m_statusLabel);

  setLayout(mainLayout);

  m_continueButton->setEnabled(false);

  connect(m_generateKeysButton, &QPushButton::clicked, this,
          &SetupPage::onGenerateKeysButtonClicked);
  connect(m_connectButton, &QPushButton::clicked, this,
          &SetupPage::onConnectButtonClicked);
  connect(m_continueButton, &QPushButton::clicked, this,
          &SetupPage::continueToChatRequested);
}

void SetupPage::setStatusText(const QString &text) {
  m_statusLabel->setText(text);
}

void SetupPage::appendStatusText(const QString &text) {
  QString current = m_statusLabel->text();
  if (current.isEmpty()) {
    m_statusLabel->setText(text);
  } else {
    m_statusLabel->setText(current + "\n" + text);
  }
}

void SetupPage::setContinueEnabled(bool enabled) {
  m_continueButton->setEnabled(enabled);
}

void SetupPage::onGenerateKeysButtonClicked() { emit generateKeysRequested(); }

void SetupPage::onConnectButtonClicked() {
  bool ok = false;
  quint16 port = m_portEdit->text().toUShort(&ok);
  if (!ok || port == 0) {
    setStatusText("Invalid port.");
    return;
  }
  QString host = m_hostEdit->text().trimmed();
  if (host.isEmpty()) {
    host = "127.0.0.1";
  }

  emit connectToServer(host, port);
}
