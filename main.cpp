#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>

#include "benchmark.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaType<Benchmark::Type>("Benchmark::Type");
    qRegisterMetaType<Benchmark::PerformanceResult>("Benchmark::PerfomanceResult");
    qRegisterMetaType<QMap<Benchmark::Type,QProgressBar*>>("QMap<Benchmark::Type,QProgressBar*>");

    QTranslator translator;
        if (translator.load(QLocale(), QLatin1String("KDiskMark"), QLatin1String("_")))
            a.installTranslator(&translator);

    MainWindow w;
    w.setFixedSize(531, 405);
    w.show();
    return a.exec();
}
