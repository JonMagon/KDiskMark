#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QString>

class AppSettings
{
    int m_loopsCount = 5;
    int m_intervalTime = 5;
    QString m_dir;

public:
    AppSettings();
    void setLoopsCount(int loops);
    int getLoopsCount();
    void setIntervalTime(int intervalTime);
    int getIntervalTime();
    void setDir(QString dir);
    QString getBenchmarkFile();
};

#endif // APPSETTINGS_H
