#include "diskdriveinfo.h"

#include <QString>
#include <QFile>
#include <QFileInfo>

QString DiskDriveInfo::getModelName(const QString &volume)
{
    QFileInfo sysClass(QFileInfo(QString("/sys/class/block/%1/..")
                                 .arg(volume.mid(volume.lastIndexOf("/"))))
                       .canonicalFilePath());

    QFile sysBlock(QString("/sys/block/%1/device/model").arg(sysClass.baseName()));

    if (!sysBlock.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    QString model = sysBlock.readAll();

    sysBlock.close();

    return model;
}
