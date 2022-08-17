#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QLocale>
#include <QString>

class QTranslator;
class QSettings;

class AppSettings : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AppSettings)

public:
    enum BenchmarkTest {
        SEQ_1,
        SEQ_2,
        RND_1,
        RND_2
    };

    struct BenchmarkParams {
        int BlockSize; // KiB
        int Queues;
        int Threads;
    };

    enum ComparisonField {
        MBPerSec,
        GBPerSec,
        IOPS,
        Latency,
    } comprasionField = MBPerSec;

    Q_ENUM(ComparisonField)

    enum PerformanceProfile {
        Default,
        Peak,
        RealWorld
    } performanceProfile = Default;

    AppSettings(QObject *parent = nullptr);

    void setupLocalization();
    QLocale locale() const;
    void setLocale(const QLocale &locale);
    static void applyLocale(const QLocale &locale);
    static QLocale defaultLocale();

    BenchmarkParams getBenchmarkParams(BenchmarkTest test);
    void setBenchmarkParams(BenchmarkTest test, int blockSize, int queues, int threads);

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
    const BenchmarkParams m_default_SEQ_1 { 1024,  8,  1 };
    const BenchmarkParams m_default_SEQ_2 { 1024,  1,  1 };
    const BenchmarkParams m_default_RND_1 {    4, 32,  1 };
    const BenchmarkParams m_default_RND_2 {    4,  1,  1 };

    const BenchmarkParams m_RealWorld_SEQ { 1024,  1,  1 };
    const BenchmarkParams m_RealWorld_RND {    4,  1,  1 };

    BenchmarkParams m_SEQ_1 = m_default_SEQ_1;
    BenchmarkParams m_SEQ_2 = m_default_SEQ_2;
    BenchmarkParams m_RND_1 = m_default_RND_1;
    BenchmarkParams m_RND_2 = m_default_RND_2;

    static QTranslator s_appTranslator;
    static QTranslator s_qtTranslator;

    QSettings *m_settings;
};

#endif // APPSETTINGS_H
