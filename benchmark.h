#ifndef FIO_H
#define FIO_H

class QProcess;
class QStringList;
class QString;

class Benchmark
{
public:
    struct PerformanceResult
    {
        float Bandwidth;
        float IOPS;
        float Latency;
    };
private:
    const char* kRW_READ = "read";
    const char* kRW_WRITE = "write";
    const char* kRW_RANDREAD = "randread";
    const char* kRW_RANDWRITE = "randwrite";
    QProcess* process_;
    QStringList* args_;
    PerformanceResult DoBenchmark(QString name, int loops, int size, int block_size, int queue_depth,
                                  int threads, const char* rw);
    PerformanceResult ParseResult();


public:
    void SEQ1M_Q8T1_Read(PerformanceResult& result, int loops);
    void SEQ1M_Q8T1_Write(PerformanceResult& result, int loops);
    void SEQ1M_Q1T1_Read(PerformanceResult& result, int loops);
    void SEQ1M_Q1T1_Write(PerformanceResult& result, int loops);
    void RND4K_Q32T16_Read(PerformanceResult& result, int loops);
    void RND4K_Q32T16_Write(PerformanceResult& result, int loops);
    void RND4K_Q1T1_Read(PerformanceResult& result, int loops);
    void RND4K_Q1T1_Write(PerformanceResult& result, int loops);

/* Singleton part */

private:
    Benchmark() {}
    ~Benchmark() {}
    Benchmark(const Benchmark&);
    Benchmark& operator=(const Benchmark&);

public:
  static Benchmark& Instance()
  {
    static Benchmark singleton;
    return singleton;
  }
};

#endif // FIO_H
