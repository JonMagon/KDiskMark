#ifndef DISKDRIVEINFO_H
#define DISKDRIVEINFO_H

class QString;

class DiskDriveInfo
{
private:
    DiskDriveInfo() {}
    ~DiskDriveInfo() {}
    DiskDriveInfo(const DiskDriveInfo&);
    DiskDriveInfo& operator=(const DiskDriveInfo&);

public:
  static DiskDriveInfo& Instance()
  {
    static DiskDriveInfo singleton;
    return singleton;
  }

  QString getDeviceByVolume(const QString &volume);
  QString getModelName(const QString &volume);
  bool isEncrypted(const QString &volume);
};

#endif // DISKDRIVEINFO_H
