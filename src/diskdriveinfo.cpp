#include "diskdriveinfo.h"

#include <QString>
#include <QFile>
#include <QFileInfo>

QString DiskDriveInfo::getDeviceByVolume(const QString &volume)
{
    QString device = QFileInfo(volume).canonicalFilePath();
    return device.mid(device.lastIndexOf("/") + 1);
}

QString DiskDriveInfo::getModelName(const QString &volume)
{
    QFileInfo sysClass(QFileInfo(QString("/sys/class/block/%1/..")
                                 .arg(getDeviceByVolume(volume)))
                       .canonicalFilePath());

    QFile sysBlock(QString("/sys/block/%1/device/model").arg(sysClass.baseName()));

    if (!sysBlock.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    QString model = sysBlock.readAll().simplified();

    sysBlock.close();

    return model;
}


bool DiskDriveInfo::isEncrypted(const QString &volume)
{
    QString device = getDeviceByVolume(volume);

    if (device.indexOf("dm") != 0)
        return false;

    QFile sysBlock(QString("/sys/block/%1/dm/uuid").arg(device));

    if (!sysBlock.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QString uuid = sysBlock.readAll().simplified();

    sysBlock.close();

    return uuid.indexOf("CRYPT") == 0;
}
