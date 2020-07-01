#include "appsettings.h"

AppSettings::AppSettings()
{

}

void AppSettings::setLoopsCount(int loops)
{
    m_loopsCount = loops;
}

int AppSettings::getLoopsCount()
{
    return m_loopsCount;
}

void AppSettings::setIntervalTime(int intervalTime)
{
    m_intervalTime = intervalTime;
}

int AppSettings::getIntervalTime()
{
    return m_intervalTime;
}

void AppSettings::setDir(QString dir)
{
    m_dir = dir;
}

QString AppSettings::getBenchmarkFile()
{
    if (m_dir.isNull())
        return QString();

    if (m_dir.endsWith("/")) {
        return m_dir + ".kdiskmark.tmp";
    }
    else {
        return m_dir + "/.kdiskmark.tmp";
    }
}
