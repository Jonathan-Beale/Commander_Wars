#include <QThread>
#include <QSslSocket>
#include <QSslError>
#include "network/sslserver.h"
#include "network/networkInterface.h"
#include "coreengine/gameconsole.h"

SslServer::SslServer(QObject *parent)
    : QSslServer{parent}
{
    // the reasons of failed connections belong into the log: without them a guest just can't join
    connect(this, &QSslServer::startedEncryptionHandshake, this, [](QSslSocket*)
    {
        CONSOLE_PRINT("SslServer: handshake started", GameConsole::eDEBUG);
    });
    connect(this, &QSslServer::errorOccurred, this, [](QSslSocket* pSocket, QAbstractSocket::SocketError error)
    {
        CONSOLE_PRINT("SslServer: socket error " + QString::number(error) + " " + (pSocket != nullptr ? pSocket->errorString() : QString()), GameConsole::eERROR);
    });
    connect(this, &QSslServer::sslErrors, this, [](QSslSocket*, const QList<QSslError> & errors)
    {
        for (const auto & error : errors)
        {
            CONSOLE_PRINT("SslServer: ssl error " + error.errorString(), GameConsole::eERROR);
        }
    });
    connect(this, &QSslServer::alertSent, this, [](QSslSocket*, QSsl::AlertLevel, QSsl::AlertType, const QString & description)
    {
        CONSOLE_PRINT("SslServer: alert sent " + description, GameConsole::eERROR);
    });
    connect(this, &QSslServer::alertReceived, this, [](QSslSocket*, QSsl::AlertLevel, QSsl::AlertType, const QString & description)
    {
        CONSOLE_PRINT("SslServer: alert received " + description, GameConsole::eERROR);
    });
}

void SslServer::incomingConnection(qintptr socket)
{
    CONSOLE_PRINT(QString("SslServer::incomingConnection as socket: ") + QString::number(socket), GameConsole::eDEBUG);
    if (NetworkInterface::usePlainTcp())
    {
        // no encryption: the socket is used like a tcp socket
        QSslSocket* pSocket = new QSslSocket(this);
        if (pSocket->setSocketDescriptor(socket))
        {
            addPendingConnection(pSocket);
        }
        else
        {
            CONSOLE_PRINT("SslServer: invalid socket " + pSocket->errorString(), GameConsole::eERROR);
            pSocket->deleteLater();
        }
        return;
    }
    QSslServer::incomingConnection(socket);
}
