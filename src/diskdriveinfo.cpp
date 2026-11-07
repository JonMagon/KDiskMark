#include "diskdriveinfo.h"

#include <QString>
#include <QFile>
#include <QFileInfo>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winioctl.h>
#include <QDir>
#include <cstring>
#include <wbemidl.h>
#include <memory>

namespace {
struct ReleaseCom { template<class T> void operator()(T *value) const { if (value) value->Release(); } };
template<class T> using ComPtr = std::unique_ptr<T, ReleaseCom>;
struct ComScope { bool active; ~ComScope() { if (active) CoUninitialize(); } };
struct Bstr {
    BSTR value;
    explicit Bstr(const wchar_t *text) : value(SysAllocString(text)) {}
    ~Bstr() { SysFreeString(value); }
    operator BSTR() const { return value; }
};

bool windowsVolumeEncrypted(const QString &volume)
{
    QString volumeName = QDir::toNativeSeparators(volume);
    if (!volumeName.startsWith(QStringLiteral("\\\\?\\Volume{"), Qt::CaseInsensitive)) {
        const auto path = QDir::toNativeSeparators(QFileInfo(volume).absoluteFilePath()).toStdWString();
        wchar_t mount[32768] = {}, name[128] = {};
        if (!GetVolumePathNameW(path.c_str(), mount, 32768)
                || !GetVolumeNameForVolumeMountPointW(mount, name, 128)) return false;
        volumeName = QString::fromWCharArray(name);
    }
    if (!volumeName.endsWith(QLatin1Char('\\'))) volumeName += QLatin1Char('\\');
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ComScope scope{SUCCEEDED(initialized)};
    if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE) return false;
    IWbemLocator *locatorRaw = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, reinterpret_cast<void **>(&locatorRaw)))) return false;
    ComPtr<IWbemLocator> locator(locatorRaw);
    IWbemServices *servicesRaw = nullptr;
    if (FAILED(locator->ConnectServer(Bstr(L"ROOT\\CIMV2\\Security\\MicrosoftVolumeEncryption"),
            nullptr, nullptr, nullptr, WBEM_FLAG_CONNECT_USE_MAX_WAIT, nullptr, nullptr, &servicesRaw))) return false;
    ComPtr<IWbemServices> services(servicesRaw);
    if (FAILED(CoSetProxyBlanket(services.get(), RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE))) return false;
    volumeName.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    volumeName.replace(QStringLiteral("'"), QStringLiteral("\\'"));
    const auto query = QStringLiteral("SELECT ConversionStatus FROM Win32_EncryptableVolume WHERE DeviceID='%1'")
        .arg(volumeName).toStdWString();
    IEnumWbemClassObject *resultsRaw = nullptr;
    if (FAILED(services->ExecQuery(Bstr(L"WQL"), Bstr(query.c_str()),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &resultsRaw))) return false;
    ComPtr<IEnumWbemClassObject> results(resultsRaw);
    if (FAILED(CoSetProxyBlanket(results.get(), RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE))) return false;
    IWbemClassObject *resultRaw = nullptr;
    ULONG count = 0;
    if (FAILED(results->Next(2000, 1, &resultRaw, &count)) || count != 1) return false;
    ComPtr<IWbemClassObject> result(resultRaw);
    VARIANT status;
    VariantInit(&status);
    const HRESULT obtained = result->Get(L"ConversionStatus", 0, &status, nullptr, nullptr);
    const bool encrypted = SUCCEEDED(obtained)
        && ((status.vt == VT_I4 && status.lVal >= 1 && status.lVal <= 5)
            || (status.vt == VT_UI4 && status.ulVal >= 1 && status.ulVal <= 5));
    VariantClear(&status);
    return encrypted;
}
}
#endif
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
#if defined(Q_OS_WIN)
    QString device = QDir::toNativeSeparators(volume);
    if (!device.startsWith(QStringLiteral("\\\\?\\Volume{"), Qt::CaseInsensitive)
            && !device.startsWith(QStringLiteral("\\\\.\\"))) {
        const std::wstring path = QDir::toNativeSeparators(QFileInfo(volume).absoluteFilePath()).toStdWString();
        wchar_t mountPoint[32768] = {};
        wchar_t volumeName[128] = {};
        if (!GetVolumePathNameW(path.c_str(), mountPoint, 32768)
                || !GetVolumeNameForVolumeMountPointW(mountPoint, volumeName, 128))
            return QString();
        device = QString::fromWCharArray(volumeName);
    }
    while (device.endsWith(QLatin1Char('\\'))) device.chop(1);
    const std::wstring nativeDevice = device.toStdWString();
    HANDLE handle = CreateFileW(nativeDevice.c_str(), 0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (handle == INVALID_HANDLE_VALUE) return QString();

    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    STORAGE_DESCRIPTOR_HEADER header = {};
    DWORD returned = 0;
    if (!DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
            &header, sizeof(header), &returned, nullptr)
            || returned < sizeof(header) || header.Size < sizeof(STORAGE_DEVICE_DESCRIPTOR)
            || header.Size > 1024 * 1024) {
        CloseHandle(handle);
        return QString();
    }
    QByteArray buffer(int(header.Size), '\0');
    const bool success = DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
        buffer.data(), DWORD(buffer.size()), &returned, nullptr);
    CloseHandle(handle);
    if (!success || returned < sizeof(STORAGE_DEVICE_DESCRIPTOR)) return QString();
    const auto descriptor = reinterpret_cast<const STORAGE_DEVICE_DESCRIPTOR *>(buffer.constData());
    const auto field = [&](DWORD offset) {
        if (offset == 0 || offset >= returned || offset >= DWORD(buffer.size())) return QString();
        const char *start = buffer.constData() + offset;
        const auto end = static_cast<const char *>(std::memchr(start, '\0', qMin(returned, DWORD(buffer.size())) - offset));
        return end ? QString::fromLocal8Bit(start, int(end - start)).simplified() : QString();
    };
    const QString vendor = field(descriptor->VendorIdOffset);
    const QString product = field(descriptor->ProductIdOffset);
    if (product.isEmpty()) return QString();
    const QString model = vendor.isEmpty() || product.startsWith(vendor, Qt::CaseInsensitive)
        ? product : vendor + QLatin1Char(' ') + product;
#elif defined(__linux__)
    QFileInfo sysClass(QFileInfo(QStringLiteral("/sys/class/block/%1/..")
                                 .arg(getDeviceByVolume(volume)))
                       .canonicalFilePath());

    QFile sysBlock(QStringLiteral("/sys/block/%1/device/model").arg(sysClass.baseName()));

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
#else
    QString model;
#endif

    return model;
}


bool DiskDriveInfo::isEncrypted(const QString &volume)
{
#ifdef Q_OS_WIN
    return windowsVolumeEncrypted(volume);
#else
    QString device = getDeviceByVolume(volume);

    if (device.indexOf("dm") != 0)
        return false;

    QFile sysBlock(QStringLiteral("/sys/block/%1/dm/uuid").arg(device));

    if (!sysBlock.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QString uuid = sysBlock.readAll().simplified();

    sysBlock.close();

    return uuid.indexOf("CRYPT") == 0;
#endif
}
