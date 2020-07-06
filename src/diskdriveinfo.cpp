#include "diskdriveinfo.h"

#include <QString>
#include <QProcess>
#include <QDebug>

QString DiskDriveInfo::getModelName(const QString &volume)
{
    QProcess *process = new QProcess();
    process->start("lsblk", QStringList() << "-no" << "pkname" << volume);
    process->waitForFinished();

    QString device = process->readAllStandardOutput().simplified();

    process->close();

    process = new QProcess();
    process->start("cat", QStringList(QString("/sys/block/%1/device/model").arg(device)));
    process->waitForFinished();

    QString model = process->readAllStandardOutput().simplified();

    process->close();

    delete process;

    return model;
}
