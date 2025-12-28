//
// Created by Baiyu on 01/12/2025.
//

#include "ChatPage.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>


ChatPage::ChatPage(QWidget *parent)
    : QWidget(parent), m_chatView(new QTextEdit(this)),
      m_inputEdit(new QLineEdit(this)),
      m_sendButton(new QPushButton("Send", this)),
      m_previewCheckBox(new QCheckBox("Enable Preview", this)) {
  m_chatView->setReadOnly(true);

  auto *inputLayout = new QHBoxLayout;
  inputLayout->addWidget(m_inputEdit);
  inputLayout->addWidget(m_sendButton);
  inputLayout->addWidget(m_previewCheckBox);

  auto *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(m_chatView);
  mainLayout->addLayout(inputLayout);

  setLayout(mainLayout);

  connect(m_sendButton, &QPushButton::clicked, this,
          &ChatPage::onSendButtonClicked);
  connect(m_inputEdit, &QLineEdit::returnPressed, this,
          &ChatPage::onSendButtonClicked);
}

void ChatPage::appendMessage(const QString &sender, const QString &text) {
  m_chatView->append(QStringLiteral("<b>%1:</b> %2")
                         .arg(sender.toHtmlEscaped(), text.toHtmlEscaped()));
}

void ChatPage::onSendButtonClicked() {
  const QString text = m_inputEdit->text().trimmed();
  if (text.isEmpty())
    return;

  emit sendMessageRequested(text);
  m_inputEdit->clear();
}

void ChatPage::appendPreviewInfo(const QString &info) {
  m_chatView->append(QStringLiteral("<i style='color:#888;'>%1</i>")
                         .arg(info.toHtmlEscaped()));
}

bool ChatPage::isPreviewEnabled() const {
  return m_previewCheckBox->isChecked();
}
