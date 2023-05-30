#ifndef HELPER_H
#define HELPER_H

#include <QObject>
#include <QLocalSocket>

class Helper : public QObject
{
    Q_OBJECT

public:
    Helper(const QString& id);
    ~Helper();

    bool connectToServer();

private:
    QLocalSocket* m_localSocket;
    quint16 m_nextBlockSize;

private:
    void sendMessageToServer(const QString& message);

private slots:
    void connected();
    void disconnected();
    void errorOccurred(QLocalSocket::LocalSocketError error);
    void readyRead();
};

#endif // HELPER_H
