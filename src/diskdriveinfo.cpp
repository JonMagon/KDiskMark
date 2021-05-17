#include "diskdriveinfo.h"

#include <QString>
#include <QFile>
#include <QFileInfo>
#ifdef __FreeBSD__
#include <sys/disk.h>
#include <sys/fcntl.h>
#include <unistd.h>
#endif

QString DiskDriveInfo::getDeviceByVolume(const QString &volume)
{
    QString device = QFileInfo(volume).canonicalFilePath();
    return device.mid(device.lastIndexOf("/") + 1);
}

QString DiskDriveInfo::getModelName(const QString &volume)
{
#if defined(__linux__)
    QFileInfo sysClass(QFileInfo(QString("/sys/class/block/%1/..")
                                 .arg(getDeviceByVolume(volume)))
                       .canonicalFilePath());

    QFile sysBlock(QString("/sys/block/%1/device/model").arg(sysClass.baseName()));

    if (!sysBlock.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    QString model(sysBlock.readAll().simplified());

    sysBlock.close();
#elif defined(__FreeBSD__)
    struct diocgattr_arg arg;

    strlcpy(arg.name, "GEOM::descr", sizeof(arg.name));
    arg.len = sizeof(arg.value.str);

    int fd = open(volume.toStdString().c_str(), O_RDONLY);
    if (fd == -1 || ioctl(fd, DIOCGATTR, &arg) == -1)
        return QString();

    QString model(arg.value.str);

    close(fd);
#endif

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
