#include "appsettings.h"

#include "cmake.h"

#include <QApplication>
#include <QCoreApplication>
#include <QTranslator>
#include <QStandardPaths>
#include <QLibraryInfo>
#include <QSettings>

QTranslator AppSettings::s_appTranslator;
QTranslator AppSettings::s_qtTranslator;

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_settings(new QSettings(this))
{
}

void AppSettings::setupLocalization()
{
    applyLocale(locale());
    QCoreApplication::installTranslator(&s_appTranslator);
    QCoreApplication::installTranslator(&s_qtTranslator);
}

QLocale AppSettings::locale() const
{
    return m_settings->value(QStringLiteral("Locale"), defaultLocale()).value<QLocale>();
}

void AppSettings::setLocale(const QLocale &locale)
{
    if (locale != this->locale()) {
        m_settings->setValue(QStringLiteral("Locale"), locale);
        applyLocale(locale);
    }
}

void AppSettings::applyLocale(const QLocale &locale)
{
    const QLocale newLocale = locale == defaultLocale() ? QLocale::system() : locale;
    QLocale::setDefault(newLocale);
    s_appTranslator.load(newLocale, QStringLiteral(PROJECT_NAME), QStringLiteral("_"), QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("translations"), QStandardPaths::LocateDirectory));
    s_qtTranslator.load(newLocale, QStringLiteral("qt"), QStringLiteral("_"), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
}

QLocale AppSettings::defaultLocale()
{
    return QLocale::c(); // C locale is used as the system language on apply
}




int AppSettings::getLoopsCount() const
{
    return m_settings->value(QStringLiteral("Benchmark/LoopsCount"), defaultLoopsCount()).toInt();
}

void AppSettings::setLoopsCount(int loopsCount)
{
    m_settings->setValue(QStringLiteral("Benchmark/LoopsCount"), loopsCount);
}

int AppSettings::defaultLoopsCount()
{
    return 5;
}

int AppSettings::getFileSize() const
{
    return m_settings->value(QStringLiteral("Benchmark/FileSize"), defaultFileSize()).toInt();
}

void AppSettings::setFileSize(int fileSize)
{
    m_settings->setValue(QStringLiteral("Benchmark/FileSize"), fileSize);
}

int AppSettings::defaultFileSize()
{
    return 1024;
}

int AppSettings::getMeasuringTime() const
{
    return m_settings->value(QStringLiteral("Benchmark/MeasuringTime"), defaultMeasuringTime()).toInt();
}

void AppSettings::setMeasuringTime(int measuringTime)
{
    m_settings->setValue(QStringLiteral("Benchmark/MeasuringTime"), measuringTime);
}

int AppSettings::defaultMeasuringTime()
{
    return 5;
}

int AppSettings::getIntervalTime() const
{
    return m_settings->value(QStringLiteral("Benchmark/IntervalTime"), defaultIntervalTime()).toInt();
}

void AppSettings::setIntervalTime(int intervalTime)
{
    m_settings->setValue(QStringLiteral("Benchmark/IntervalTime"), intervalTime);
}

int AppSettings::defaultIntervalTime()
{
    return 5;
}

int AppSettings::getRandomReadPercentage() const
{
    return m_settings->value(QStringLiteral("Benchmark/RandomReadPercentage"), defaultRandomReadPercentage()).toInt();
}

void AppSettings::setRandomReadPercentage(int randomReadPercentage)
{
    m_settings->setValue(QStringLiteral("Benchmark/RandomReadPercentage"), randomReadPercentage);
}

int AppSettings::defaultRandomReadPercentage()
{
    return 70;
}

bool AppSettings::getFlusingCacheState() const
{
    return m_settings->value(QStringLiteral("Benchmark/FlushingCache"), defaultFlushingCacheState()).toBool();
}

void AppSettings::setFlushingCacheState(bool state)
{
    m_settings->setValue(QStringLiteral("Benchmark/FlushingCache"), state);
}

bool AppSettings::defaultFlushingCacheState()
{
    return true;
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
