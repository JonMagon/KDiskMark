#include "appsettings.h"

#include "cmake.h"

#include <QApplication>
#include <QCoreApplication>
#include <QTranslator>
#include <QStandardPaths>
#include <QLibraryInfo>

QTranslator AppSettings::s_appTranslator;
QTranslator AppSettings::s_qtTranslator;

void AppSettings::setupLocalization()
{
    setLocale(QLocale());
    QCoreApplication::installTranslator(&s_appTranslator);
    QCoreApplication::installTranslator(&s_qtTranslator);
}

void AppSettings::setLocale(const QLocale locale)
{
    if (locale == QLocale::AnyLanguage)
        QLocale::setDefault(QLocale::system());
    else
        QLocale::setDefault(QLocale(locale));

    s_appTranslator.load(QLocale(), qAppName(), QStringLiteral("_"),
                         QStandardPaths::locate(QStandardPaths::AppDataLocation,
                                                QStringLiteral("translations"),
                                                QStandardPaths::LocateDirectory));
    s_qtTranslator.load(QLocale(), QStringLiteral("qt"), QStringLiteral("_"),
                        QLibraryInfo::location(QLibraryInfo::TranslationsPath));
}

QLocale AppSettings::defaultLocale()
{
    return QLocale::AnyLanguage;
}

void AppSettings::setLoopsCount(int loops)
{
    m_loopsCount = loops;
}

int AppSettings::getLoopsCount()
{
    return m_loopsCount;
}

void AppSettings::setFileSize(int size)
{
    m_fileSize = size;
}

int AppSettings::getFileSize()
{
    return m_fileSize;
}

void AppSettings::setIntervalTime(int intervalTime)
{
    m_intervalTime = intervalTime;
}

int AppSettings::getIntervalTime()
{
    return m_intervalTime;
}

void AppSettings::setDir(const QString &dir)
{
    m_dir = dir;
}

void AppSettings::setRandomReadPercentage(float percentage)
{
    m_percentage = percentage;
}

int AppSettings::getRandomReadPercentage()
{
    return m_percentage;
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

bool AppSettings::isMixed()
{
    return m_mixedState;
}

void AppSettings::setMixed(bool state)
{
    m_mixedState = state;
}

void AppSettings::restoreDefaultBenchmarkParams()
{
    m_SEQ_1 = m_default_SEQ_1;
    m_SEQ_2 = m_default_SEQ_2;
    m_RND_1 = m_default_RND_1;
    m_RND_2 = m_default_RND_2;
}

AppSettings::BenchmarkParams AppSettings::getBenchmarkParams(BenchmarkTest test)
{
    switch (test)
    {
    case SEQ_1:
        return performanceProfile != PerformanceProfile::RealWorld ? m_SEQ_1 : m_RealWorld_SEQ;
    case SEQ_2:
        return m_SEQ_2;
    case RND_1:
        return performanceProfile != PerformanceProfile::RealWorld ? m_RND_1 : m_RealWorld_RND;
    case RND_2:
        return m_RND_2;
    }
    Q_UNREACHABLE();
}

void AppSettings::setBenchmarkParams(BenchmarkTest test, int blockSize, int queues, int threads)
{
    switch (test)
    {
    case SEQ_1:
        m_SEQ_1 = { blockSize, queues, threads };
        break;
    case SEQ_2:
        m_SEQ_2 = { blockSize, queues, threads };
        break;
    case RND_1:
        m_RND_1 = { blockSize, queues, threads };
        break;
    case RND_2:
        m_RND_2 = { blockSize, queues, threads };
        break;
    }
}
