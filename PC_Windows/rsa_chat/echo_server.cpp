//
// Created by Baiyu on 01/12/2025.
//
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QDebug>

class EchoServer : public QTcpServer {
public:
    explicit EchoServer(QObject* parent = nullptr)
        : QTcpServer(parent)
    {}

protected:
    void incomingConnection(qintptr socketDescriptor) override {
        auto* socket = new QTcpSocket(this);
        if (!socket->setSocketDescriptor(socketDescriptor)) {
            qWarning() << "Failed to set socket descriptor:" << socket->errorString();
            socket->deleteLater();
            return;
        }

        qInfo() << "Client connected from"
                << socket->peerAddress().toString()
                << ":" << socket->peerPort();

        QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
            QByteArray data = socket->readAll();
            qInfo() << "Received" << data;
            socket->write(data);
            socket->flush();
        });

        QObject::connect(socket, &QTcpSocket::disconnected,
                         socket, &QObject::deleteLater);
    }
};

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    EchoServer server;
    const quint16 port = 12345;  // match your client default
    if (!server.listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to listen on port" << port << ":" << server.errorString();
        return 1;
    }

    qInfo() << "Echo server listening on port" << port << "...";
    return app.exec();
}
