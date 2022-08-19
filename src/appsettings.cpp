#include "appsettings.h"

#include "cmake.h"

#include <QApplication>
#include <QCoreApplication>
#include <QTranslator>
#include <QStandardPaths>
#include <QLibraryInfo>
#include <QSettings>
#include <QMetaEnum>

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
    return QLocale::c();
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

Global::BenchmarkParams AppSettings::getBenchmarkParams(Global::BenchmarkTest test, Global::PerformanceProfile profile) const
{
    Global::BenchmarkParams defaultSet = defaultBenchmarkParams(test, profile);
    if (profile == Global::PerformanceProfile::RealWorld) return defaultSet;

    QString settingKey = QStringLiteral("Benchmark/Params/%1/%2/%3")
            .arg(QMetaEnum::fromType<Global::PerformanceProfile>().valueToKey(profile))
            .arg(QMetaEnum::fromType<Global::BenchmarkTest>().valueToKey(test));

    return {
        (Global::BenchmarkIOPattern)m_settings->value(settingKey.arg("Pattern"), defaultSet.Pattern).toInt(),
        m_settings->value(settingKey.arg("BlockSize"), defaultSet.BlockSize).toInt(),
        m_settings->value(settingKey.arg("Queues"), defaultSet.Queues).toInt(),
        m_settings->value(settingKey.arg("Threads"), defaultSet.Threads).toInt()
    };
}

void AppSettings::setBenchmarkParams(Global::BenchmarkTest test, Global::PerformanceProfile profile, Global::BenchmarkParams params)
{
    QString settingKey = QStringLiteral("Benchmark/Params/%1/%2/%3")
            .arg(QMetaEnum::fromType<Global::PerformanceProfile>().valueToKey(profile))
            .arg(QMetaEnum::fromType<Global::BenchmarkTest>().valueToKey(test));

    m_settings->setValue(settingKey.arg("Pattern"), params.Pattern);
    m_settings->setValue(settingKey.arg("BlockSize"), params.BlockSize);
    m_settings->setValue(settingKey.arg("Queues"), params.Queues);
    m_settings->setValue(settingKey.arg("Threads"), params.Threads);
}

Global::BenchmarkParams AppSettings::defaultBenchmarkParams(Global::BenchmarkTest test, Global::PerformanceProfile profile)
{
    switch (profile)
    {
        case Global::PerformanceProfile::Default:
            switch (test)
            {
            case Global::BenchmarkTest::Test_1:
                return { Global::BenchmarkIOPattern::SEQ, 1024,  8,  1 };
            case Global::BenchmarkTest::Test_2:
                return { Global::BenchmarkIOPattern::SEQ, 1024,  1,  1 };
            case Global::BenchmarkTest::Test_3:
                return { Global::BenchmarkIOPattern::RND,    4, 32,  1 };
            case Global::BenchmarkTest::Test_4:
                return { Global::BenchmarkIOPattern::RND,    4,  1,  1 };
            }
            break;
        case Global::PerformanceProfile::Peak:
            switch (test)
            {
            case Global::BenchmarkTest::Test_1:
                return { Global::BenchmarkIOPattern::SEQ, 1024,  8,  1 };
            case Global::BenchmarkTest::Test_2:
                return { Global::BenchmarkIOPattern::RND,    4, 32,  1 };
            }
        case Global::PerformanceProfile::Demo:
            switch (test)
            {
            case Global::BenchmarkTest::Test_1:
                return { Global::BenchmarkIOPattern::SEQ, 1024,  8,  1 };
            }
        case Global::PerformanceProfile::RealWorld:
            switch (test)
            {
            case Global::BenchmarkTest::Test_1:
                return { Global::BenchmarkIOPattern::SEQ, 1024,  1,  1 };
            case Global::BenchmarkTest::Test_2:
                return { Global::BenchmarkIOPattern::RND,    4,  1,  1 };
            }
    }
    Q_UNREACHABLE();
}
