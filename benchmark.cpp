#include "benchmark.h"

#include <QProcess>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

Benchmark::PerformanceResult Benchmark::DoBenchmark(QString name, int loops, int size, int block_size,
                                                    int queue_depth, int threads, const char* rw)
{
    process_ = new QProcess();
    process_->start("fio", QStringList()
                    << "--stonewall"
                    << "--output-format=json"
                    << "--ioengine=libaio"
                    << "--direct=1"
                    << "--filename=/home/jonmagon/fiotest.tmp"
                    << QString("--name=%1").arg(name)
                    << QString("--loops=%1").arg(loops)
                    << QString("--size=%1k").arg(size)
                    << QString("--bs=%1k").arg(block_size)
                    << QString("--rw=%1").arg(rw)
                    << QString("--iodepth=%1").arg(queue_depth)
                    << QString("--numjobs=%1").arg(threads)
                    );
    process_->waitForFinished();

    return ParseResult();
}

Benchmark::PerformanceResult Benchmark::ParseResult()
{
    QJsonDocument jsonResponse = QJsonDocument::fromJson(QString(process_->readAllStandardOutput()).toUtf8());
    QJsonObject jsonObject = jsonResponse.object();
    QJsonArray jobs = jsonObject["jobs"].toArray();

    if (jobs.count() == 0)
        throw 1;

    PerformanceResult result = PerformanceResult();

    QJsonValue job = jobs.takeAt(0);

    if (job["jobname"].toString().contains("Read")) {
        QJsonObject readResults = job["read"].toObject();

        result.Bandwidth = readResults.value("bw").toInt() / 1000.0; // to kbytes
        result.IOPS = readResults.value("iops").toDouble();
        result.Latency = readResults["lat_ns"].toObject().value("mean").toDouble() / 1000.0; // from nsec to usec
    } else if (job["jobname"].toString().contains("Write")) {
        QJsonObject writeResults = job["write"].toObject();

        result.Bandwidth = writeResults.value("bw").toInt() / 1000.0; // to kbytes
        result.IOPS = writeResults.value("iops").toDouble();
        result.Latency = writeResults["lat_ns"].toObject().value("mean").toDouble() / 1000.0; // from nsec to usec
    }

    return result;
}

void Benchmark::SEQ1M_Q8T1_Read(Benchmark::PerformanceResult& result, int loops)
{
    result = DoBenchmark(__func__, loops, 32 * 1024, 1024, 8, 1, kRW_READ);
}

void Benchmark::SEQ1M_Q8T1_Write(Benchmark::PerformanceResult& result, int loops)
{
    result = DoBenchmark(__func__, loops, 32 * 1024, 1024, 8, 1, kRW_WRITE);
}

void Benchmark::SEQ1M_Q1T1_Read(Benchmark::PerformanceResult& result, int loops)
{
    result = DoBenchmark(__func__, loops, 32 * 1024, 1024, 1, 1, kRW_READ);
}

void Benchmark::SEQ1M_Q1T1_Write(Benchmark::PerformanceResult& result, int loops)
{
    result = DoBenchmark(__func__, loops, 32 * 1024, 1024, 1, 1, kRW_WRITE);
}

void Benchmark::RND4K_Q32T16_Read(Benchmark::PerformanceResult& result, int loops)
{
    result = DoBenchmark(__func__, loops, 32 * 1024, 4, 32, 16, kRW_RANDREAD);
}

void Benchmark::RND4K_Q32T16_Write(Benchmark::PerformanceResult& result, int loops)
{
    result = DoBenchmark(__func__, loops, 32 * 1024, 4, 32, 16, kRW_RANDWRITE);
}

void Benchmark::RND4K_Q1T1_Read(Benchmark::PerformanceResult& result, int loops)
{
    result = DoBenchmark(__func__, loops, 32 * 1024, 4, 1, 1, kRW_RANDREAD);
}

void Benchmark::RND4K_Q1T1_Write(Benchmark::PerformanceResult& result, int loops)
{
    result = DoBenchmark(__func__, loops, 32 * 1024, 4, 1, 1, kRW_RANDWRITE);
}

