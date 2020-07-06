#include "diskdriveinfo.h"

#include <QString>
#include <QProcess>
#include <QFile>

QString DiskDriveInfo::getModelName(const QString &volume)
{
    QProcess *process = new QProcess();
    process->start("lsblk", QStringList() << "-no" << "pkname" << volume);
    process->waitForFinished();

    QString device = process->readAllStandardOutput().simplified();

    process->close();
    delete process;

    QFile sysBlock(QString("/sys/block/%1/device/model").arg(device));

    if (!sysBlock.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    QString model = sysBlock.readAll();

    sysBlock.close();

    return model;
}
