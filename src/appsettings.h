#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QLocale>
#include <QString>

#if defined(BUILD_WITH_PAGECACHE_CLEARING_SUPPORT) && !defined(PERFORM_PAGECACHE_CLEARING_USING_KF5AUTH)
#include <unistd.h>
#endif

class QTranslator;

class AppSettings : public QObject
{
    Q_OBJECT

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

    AppSettings();

    void setupLocalization();
    QLocale getLocale();
    void setLocale(const QLocale locale);
    static QLocale defaultLocale();
#if defined(BUILD_WITH_PAGECACHE_CLEARING_SUPPORT) && !defined(PERFORM_PAGECACHE_CLEARING_USING_KF5AUTH)
    bool isRunningAsRoot();
#endif

    BenchmarkParams getBenchmarkParams(BenchmarkTest test);
    void setBenchmarkParams(BenchmarkTest test, int blockSize, int queues, int threads);
    void restoreDefaultBenchmarkParams();
    void setLoopsCount(int loops);
    int getLoopsCount();
    void setFileSize(int size);
    int getFileSize();
    void setIntervalTime(int intervalTime);
    int getIntervalTime();
    void setDir(const QString &dir);
    void setRandomReadPercentage(float percentage);
    int getRandomReadPercentage();
    QString getBenchmarkFile();
    void setMixed(bool state);
    bool isMixed();
    void setFlushingCacheState(bool state);
    bool shouldFlushCache();

private:
    int m_loopsCount = 5;
    int m_fileSize = 1024;
    int m_intervalTime = 5;
    int m_percentage = 70;
    QString m_dir;
    bool m_mixedState = false;
    bool m_shouldFlushCache = false;

    const BenchmarkParams m_default_SEQ_1 { 1024,  8,  1 };
    const BenchmarkParams m_default_SEQ_2 { 1024,  1,  1 };
    const BenchmarkParams m_default_RND_1 {    4, 32, 16 };
    const BenchmarkParams m_default_RND_2 {    4,  1,  1 };

    const BenchmarkParams m_RealWorld_SEQ { 1024,  1,  1 };
    const BenchmarkParams m_RealWorld_RND {    4,  1,  1 };

    BenchmarkParams m_SEQ_1 = m_default_SEQ_1;
    BenchmarkParams m_SEQ_2 = m_default_SEQ_2;
    BenchmarkParams m_RND_1 = m_default_RND_1;
    BenchmarkParams m_RND_2 = m_default_RND_2;

    static QTranslator s_appTranslator;
    static QTranslator s_qtTranslator;

#if defined(BUILD_WITH_PAGECACHE_CLEARING_SUPPORT) && !defined(PERFORM_PAGECACHE_CLEARING_USING_KF5AUTH)
    bool m_runningAsRoot;
#endif
};

#endif // APPSETTINGS_H
