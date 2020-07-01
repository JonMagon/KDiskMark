#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QString>

class AppSettings
{
    int m_loopsCount = 5;
    int m_intervalTime = 5;
    QString m_dir;

public:
    struct BenchmarkParams {
        int BlockSize; // KiB
        int Queues;
        int Threads;
    };

    const BenchmarkParams default_SEQ_1 { 1024,  8,  1 };
    const BenchmarkParams default_SEQ_2 { 1024,  1,  1 };
    const BenchmarkParams default_RND_1 {    4, 32, 16 };
    const BenchmarkParams default_RND_2 {    4,  1,  1 };

    BenchmarkParams SEQ_1 = default_SEQ_1;
    BenchmarkParams SEQ_2 = default_SEQ_2;
    BenchmarkParams RND_1 = default_RND_1;
    BenchmarkParams RND_2 = default_RND_2;

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
