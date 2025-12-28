//
// Created by Baiyu on 01/12/2025.
//

#pragma once

#include <QWidget>

class QTextEdit;
class QLineEdit;
class QPushButton;
class QCheckBox;

class ChatPage : public QWidget {
  Q_OBJECT
public:
  explicit ChatPage(QWidget *parent = nullptr);

  void appendMessage(const QString &sender, const QString &text);
  void appendPreviewInfo(const QString &info);
  bool isPreviewEnabled() const;

signals:
  void sendMessageRequested(const QString &text);

private slots:
  void onSendButtonClicked();

private:
  QTextEdit *m_chatView;
  QLineEdit *m_inputEdit;
  QPushButton *m_sendButton;
  QCheckBox *m_previewCheckBox;
};
