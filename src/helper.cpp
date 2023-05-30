#include "helper.h"

#include <QCoreApplication>
#include <QDataStream>
#include <QFile>

Helper::Helper(const QString& id): m_localSocket(new QLocalSocket(this)), m_nextBlockSize(0) {
    m_localSocket->setServerName("kdiskmark" + id);

    connect(m_localSocket, &QLocalSocket::connected, this, &Helper::connected);
    connect(m_localSocket, &QLocalSocket::disconnected, this, &Helper::disconnected);
    connect(m_localSocket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::errorOccurred), this, &Helper::errorOccurred);
    connect(m_localSocket, &QLocalSocket::readyRead, this, &Helper::readyRead);
}

bool Helper::connectToServer()
{
    m_localSocket->connectToServer();
    return m_localSocket->waitForConnected();
}

Helper::~Helper()
{
    delete m_localSocket;
    m_localSocket = nullptr;
}

void Helper::connected()
{
    qInfo() << "Connected to server";
}

void Helper::disconnected()
{
    qInfo() << "Disconnected from server";
    qApp->quit();
}

void Helper::errorOccurred(QLocalSocket::LocalSocketError error)
{
    qCritical() << "Error:" << m_localSocket->errorString();
}

void Helper::readyRead()
{
    QDataStream in(m_localSocket);
    for (;;) {
        if (!m_nextBlockSize) {
            if (m_localSocket->bytesAvailable() < (int)sizeof(quint16))
                break;
        }
        in >> m_nextBlockSize;

        if (m_localSocket->bytesAvailable() < m_nextBlockSize)
            break;

        QString message;
        in >> message;

        qInfo() << "C<-" << message;

        if (message == "HALT") {
            qApp->exit();
        }
        else if (message == "FLUSHCACHE") {
            QFile file("/proc/sys/vm/drop_caches");

            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.write("1");
                file.close();

                sendMessageToServer("OK");
            }
            else {
                sendMessageToServer("ERROR " + file.errorString());
            }
        }

        m_nextBlockSize = 0;
    }
}

void Helper::sendMessageToServer(const QString& message)
{
    if (!m_localSocket) return;

    QByteArray array;
    QDataStream out(&array, QIODevice::WriteOnly);
    out << quint16(0) << message;
    out.device()->seek(0);
    out << quint16(array.size() - sizeof(quint16));
    m_localSocket->write(array);

    qInfo() << "C->" << message;
}
