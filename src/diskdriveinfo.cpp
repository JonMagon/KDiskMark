#include "diskdriveinfo.h"

#include <QString>
#include <QFile>
#include <QFileInfo>
#if defined(__linux__)
// cppcheck-suppress missingIncludeSystem ; Qt's headers live outside cppcheck's search path
#include <QDir>
#endif
#ifdef __FreeBSD__
#include <sys/disk.h>
#include <sys/fcntl.h>
#include <unistd.h>
#endif

#if defined(__linux__)
namespace {

// Resolve a PCI vendor:device pair to a human-readable "ShortVendor DeviceName"
// (e.g. 0x1022:0xb000 -> "AMD RAID Bottom Device") using the system hwids
// database shipped by the "hwdata"/"pciutils" packages. The vendor's short name
// is taken from the trailing "[...]" alias when present, so we get "AMD" rather
// than the verbose "Advanced Micro Devices, Inc. [AMD]". Returns an empty string
// if the database is missing or the ids are not listed.
QString pciDeviceName(quint16 vendorId, quint16 deviceId)
{
    static const QStringList paths = {
        QStringLiteral("/usr/share/hwdata/pci.ids"),
        QStringLiteral("/usr/share/misc/pci.ids"),
    };

    const QString vendorPrefix = QStringLiteral("%1  ").arg(vendorId, 4, 16, QLatin1Char('0'));
    const QString devicePrefix = QStringLiteral("%1  ").arg(deviceId, 4, 16, QLatin1Char('0'));

    for (const QString &path : paths) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        QString shortVendor;
        bool inVendor = false;

        while (!file.atEnd()) {
            // Read one line, dropping the trailing newline but keeping the leading
            // tabs that distinguish vendor / device / subsystem entries.
            QString line = QString::fromUtf8(file.readLine());
            while (line.endsWith(QLatin1Char('\n')) || line.endsWith(QLatin1Char('\r')))
                line.chop(1);
            if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
                continue;

            if (!line.startsWith(QLatin1Char('\t'))) {
                // Vendor line, e.g. "1022  Advanced Micro Devices, Inc. [AMD]".
                // The list is sorted by vendor id, so once we leave our vendor's
                // block the device cannot appear later: stop scanning.
                if (inVendor)
                    return QString();
                if (line.startsWith(vendorPrefix)) {
                    inVendor = true;
                    const QString vendorName = line.mid(vendorPrefix.length()).trimmed();
                    const int lb = vendorName.lastIndexOf(QLatin1Char('['));
                    const int rb = vendorName.lastIndexOf(QLatin1Char(']'));
                    shortVendor = (lb >= 0 && rb > lb)
                            ? vendorName.mid(lb + 1, rb - lb - 1)
                            : vendorName;
                }
            } else if (inVendor && !line.startsWith(QStringLiteral("\t\t"))) {
                // Device line, e.g. "\tb000  RAID Bottom Device" (two-tab lines
                // are subsystem entries and are skipped by the check above).
                const QString device = line.mid(1);
                if (device.startsWith(devicePrefix)) {
                    const QString deviceName = device.mid(devicePrefix.length()).trimmed();
                    return shortVendor.isEmpty()
                            ? deviceName
                            : shortVendor + QLatin1Char(' ') + deviceName;
                }
            }
        }
        return QString();
    }

    return QString();
}

// Read a "0x1234"-style hex id from a sysfs attribute. Returns 0 on failure.
quint16 readSysfsHex(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0;
    return file.readAll().simplified().toUShort(nullptr, 16);
}

// Firmware/driver-backed RAID (e.g. AMD RAIDXpert2) exposes a single virtual
// array block device with no link back to the controllers that make it up. Find
// the RAID controller among the PCI devices and return its human name, e.g.
// "AMD RAID Bottom Device". Returns an empty string if none is identified.
QString firmwareRaidControllerName()
{
    QDir pciDir(QStringLiteral("/sys/bus/pci/devices"));
    const auto slotDirs = pciDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &slot : slotDirs) {
        const QString base = pciDir.filePath(slot);

        QString driver = QFileInfo(base + QStringLiteral("/driver")).symLinkTarget();
        driver = driver.mid(driver.lastIndexOf(QLatin1Char('/')) + 1);

        // Accept either a known firmware-RAID member driver, or the generic PCI
        // "RAID bus controller" class (0x0104xx) used by other RAID HBAs.
        QFile classFile(base + QStringLiteral("/class"));
        QString pciClass;
        if (classFile.open(QIODevice::ReadOnly | QIODevice::Text))
            pciClass = classFile.readAll().simplified();

        // Known firmware-RAID member drivers (AMD RAIDXpert2).
        const bool isRaidDriver = driver == QLatin1String("rcbottom")
                               || driver == QLatin1String("rcraid");
        if (!isRaidDriver && !pciClass.startsWith(QStringLiteral("0x0104")))
            continue;

        const quint16 vendorId = readSysfsHex(base + QStringLiteral("/vendor"));
        const quint16 deviceId = readSysfsHex(base + QStringLiteral("/device"));
        const QString name = pciDeviceName(vendorId, deviceId);
        if (!name.isEmpty())
            return name;
    }

    return QString();
}

} // namespace
#endif

QString DiskDriveInfo::getDeviceByVolume(const QString &volume)
{
    QString device = QFileInfo(volume).canonicalFilePath();
    return device.mid(device.lastIndexOf("/") + 1);
}

QString DiskDriveInfo::getModelName(const QString &volume)
{
#if defined(__linux__)
    QFileInfo sysClass(QFileInfo(QStringLiteral("/sys/class/block/%1/..")
                                 .arg(getDeviceByVolume(volume)))
                       .canonicalFilePath());

    const QString blockName = sysClass.baseName();

    QFile sysBlock(QStringLiteral("/sys/block/%1/device/model").arg(blockName));

    if (sysBlock.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString model(sysBlock.readAll().simplified());
        sysBlock.close();
        if (!model.isEmpty())
            return model;
    }

    // RAID arrays and other virtual devices have no device/model node, which would
    // otherwise leave the field blank. Provide a descriptive fallback instead.

    // Firmware/driver-backed RAID (e.g. AMD RAIDXpert2 "rcraid", Intel RST "isw")
    // presents a single virtual array device and hides its members. Name the
    // controller from the PCI database when we can, e.g.
    // "RAID Array: AMD RAID Bottom Device", otherwise a plain "RAID Array".
    if (blockName.contains(QStringLiteral("raid"), Qt::CaseInsensitive)) {
        const QString controller = firmwareRaidControllerName();
        return controller.isEmpty()
                ? QStringLiteral("RAID Array")
                : QStringLiteral("RAID Array: %1").arg(controller);
    }

    return QString();
#elif defined(__FreeBSD__)
    struct diocgattr_arg arg;

    strlcpy(arg.name, "GEOM::descr", sizeof(arg.name));
    arg.len = sizeof(arg.value.str);

    int fd = open(volume.toStdString().c_str(), O_RDONLY);
    if (fd == -1 || ioctl(fd, DIOCGATTR, &arg) == -1)
        return QString();

    QString model(arg.value.str);

    close(fd);

    return model;
#else
    Q_UNUSED(volume)
#endif

    return QString();
}


bool DiskDriveInfo::isEncrypted(const QString &volume)
{
    QString device = getDeviceByVolume(volume);

    if (device.indexOf("dm") != 0)
        return false;

    QFile sysBlock(QStringLiteral("/sys/block/%1/dm/uuid").arg(device));

    if (!sysBlock.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QString uuid = sysBlock.readAll().simplified();

    sysBlock.close();

    return uuid.indexOf("CRYPT") == 0;
}
