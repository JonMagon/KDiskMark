#include "appsettings.h"

#include "cmake.h"

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

    QCoreApplication::removeTranslator(&s_appTranslator);
    QCoreApplication::removeTranslator(&s_qtTranslator);

    if (newLocale.language() != QLocale::English) {
        bool appTranslatorLoaded = s_appTranslator.load(newLocale, QStringLiteral(PROJECT_NAME), QStringLiteral("_"), QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("translations"), QStandardPaths::LocateDirectory));
        if (appTranslatorLoaded) {
            QCoreApplication::installTranslator(&s_appTranslator);
        } else {
            qWarning() << "Failed to load application translations for locale" << newLocale;
        }
    }

    bool qtTranslatorLoaded = s_qtTranslator.load(newLocale, QStringLiteral("qt"), QStringLiteral("_"), QLibraryInfo::path(QLibraryInfo::TranslationsPath));
    if (qtTranslatorLoaded) {
        QCoreApplication::installTranslator(&s_qtTranslator);
    } else {
        qWarning() << "Failed to load Qt translations for locale" << newLocale;
    }
}

QLocale AppSettings::defaultLocale()
{
    return QLocale::c();
}

Global::PerformanceProfile AppSettings::getPerformanceProfile() const
{
    return (Global::PerformanceProfile)m_settings->value(QStringLiteral("Benchmark/PerformanceProfile"), defaultPerformanceProfile()).toInt();
}

void AppSettings::setPerformanceProfile(Global::PerformanceProfile performanceProfile)
{
    m_settings->setValue(QStringLiteral("Benchmark/PerformanceProfile"), performanceProfile);
}

Global::PerformanceProfile AppSettings::defaultPerformanceProfile()
{
    return Global::PerformanceProfile::Default;
}

bool AppSettings::getMixedState() const
{
    return m_settings->value(QStringLiteral("Benchmark/Mixed"), defaultMixedState()).toBool();
}

void AppSettings::setMixedState(bool state)
{
    m_settings->setValue(QStringLiteral("Benchmark/Mixed"), state);
}

bool AppSettings::defaultMixedState()
{
    return false;
}

Global::BenchmarkMode AppSettings::getBenchmarkMode() const
{
    return (Global::BenchmarkMode)m_settings->value(QStringLiteral("Benchmark/Mode"), defaultBenchmarkMode()).toInt();
}

void AppSettings::setBenchmarkMode(Global::BenchmarkMode benchmarkMode)
{
    m_settings->setValue(QStringLiteral("Benchmark/Mode"), benchmarkMode);
}

Global::BenchmarkMode AppSettings::defaultBenchmarkMode()
{
    return Global::BenchmarkMode::ReadWriteMix;
}

Global::BenchmarkTestData AppSettings::getBenchmarkTestData() const
{
    return (Global::BenchmarkTestData)m_settings->value(QStringLiteral("Benchmark/TestData"), defaultBenchmarkTestData()).toInt();
}

void AppSettings::setBenchmarkTestData(Global::BenchmarkTestData benchmarkTestData)
{
    m_settings->setValue(QStringLiteral("Benchmark/TestData"), benchmarkTestData);
}

Global::BenchmarkTestData AppSettings::defaultBenchmarkTestData()
{
    return Global::BenchmarkTestData::Random;
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

bool AppSettings::getCacheBypassState() const
{
    return m_settings->value(QStringLiteral("Benchmark/BypassCache"), defaultCacheBypassState()).toBool();
}

void AppSettings::setCacheBypassState(bool state)
{
    m_settings->setValue(QStringLiteral("Benchmark/BypassCache"), state);
}

bool AppSettings::defaultCacheBypassState()
{
    return true;
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

Global::ComparisonUnit AppSettings::getComparisonUnit() const
{
    return (Global::ComparisonUnit)m_settings->value(QStringLiteral("Interface/ComparisonUnit"), defaultComparisonUnit()).toInt();
}

void AppSettings::setComparisonUnit(Global::ComparisonUnit comparisonUnit)
{
    m_settings->setValue(QStringLiteral("Interface/ComparisonUnit"), comparisonUnit);
}

Global::ComparisonUnit AppSettings::defaultComparisonUnit()
{
    return Global::ComparisonUnit::MBPerSec;
}

Global::Theme AppSettings::getTheme() const
{
    return (Global::Theme)m_settings->value(QStringLiteral("Interface/Theme"), defaultTheme()).toInt();
}

void AppSettings::setTheme(Global::Theme theme)
{
    m_settings->setValue(QStringLiteral("Interface/Theme"), theme);
}

Global::Theme AppSettings::defaultTheme()
{
    return Global::Theme::UseFusion;
}

Global::BenchmarkParams AppSettings::getBenchmarkParams(Global::BenchmarkTest test, Global::PerformanceProfile profile) const
{
    Global::BenchmarkParams defaultSet = defaultBenchmarkParams(test, profile, Global::BenchmarkPreset::Standard);
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

Global::BenchmarkParams AppSettings::defaultBenchmarkParams(Global::BenchmarkTest test, Global::PerformanceProfile profile, Global::BenchmarkPreset preset)
{
    switch (profile)
    {
        case Global::PerformanceProfile::Default:
            switch (test)
            {
            case Global::BenchmarkTest::Test_1:
                return { Global::BenchmarkIOPattern::SEQ, 1024,  8,  1 };
            case Global::BenchmarkTest::Test_2:
                if (preset == Global::BenchmarkPreset::Standard)
                return { Global::BenchmarkIOPattern::SEQ, 1024,  1,  1 };
                else
                return { Global::BenchmarkIOPattern::SEQ,  128, 32,  1 };
            case Global::BenchmarkTest::Test_3:
                if (preset == Global::BenchmarkPreset::Standard)
                return { Global::BenchmarkIOPattern::RND,    4, 32,  1 };
                else
                return { Global::BenchmarkIOPattern::RND,    4, 32, 16 };
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
                if (preset == Global::BenchmarkPreset::Standard)
                return { Global::BenchmarkIOPattern::RND,    4, 32,  1 };
                else
                return { Global::BenchmarkIOPattern::RND,    4, 32, 16 };
            }
        case Global::PerformanceProfile::RealWorld:
            switch (test)
            {
            case Global::BenchmarkTest::Test_1:
                return { Global::BenchmarkIOPattern::SEQ, 1024,  1,  1 };
            case Global::BenchmarkTest::Test_2:
                return { Global::BenchmarkIOPattern::RND,    4,  1,  1 };
            }
        case Global::PerformanceProfile::Demo:
            switch (test)
            {
            case Global::BenchmarkTest::Test_1:
                return { Global::BenchmarkIOPattern::SEQ, 1024,  8,  1 };
            }
    }
    Q_UNREACHABLE();
}
