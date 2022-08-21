#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QLocale>
#include <QString>

#include "global.h"

class QTranslator;
class QSettings;

class AppSettings : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AppSettings)

public:
    AppSettings(QObject *parent = nullptr);

    void setupLocalization();
    QLocale locale() const;
    void setLocale(const QLocale &locale);
    static void applyLocale(const QLocale &locale);
    static QLocale defaultLocale();

    Global::BenchmarkParams getBenchmarkParams(Global::BenchmarkTest test, Global::PerformanceProfile profile = Global::PerformanceProfile::Default) const;
    void setBenchmarkParams(Global::BenchmarkTest test, Global::PerformanceProfile profile, Global::BenchmarkParams params);
    static Global::BenchmarkParams defaultBenchmarkParams(Global::BenchmarkTest test, Global::PerformanceProfile profile, Global::BenchmarkPreset preset);

    int getLoopsCount() const;
    void setLoopsCount(int loopsCount);
    static int defaultLoopsCount();

    int getFileSize() const;
    void setFileSize(int fileSize);
    static int defaultFileSize();

    int getMeasuringTime() const;
    void setMeasuringTime(int measuringTime);
    static int defaultMeasuringTime();

    int getIntervalTime() const;
    void setIntervalTime(int intervalTime);
    static int defaultIntervalTime();

    int getRandomReadPercentage() const;
    void setRandomReadPercentage(int randomReadPercentage);
    static int defaultRandomReadPercentage();

    bool getFlusingCacheState() const;
    void setFlushingCacheState(bool flushingCacheState);
    static bool defaultFlushingCacheState();

private:
    static QTranslator s_appTranslator;
    static QTranslator s_qtTranslator;

    QSettings *m_settings;
};

#endif // APPSETTINGS_H
