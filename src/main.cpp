#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>
#include "benchmark.h"
#include "cmake.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationVersion(QStringLiteral("%1.%2.%3").arg(PROJECT_VERSION_MAJOR)
                                            .arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH));

    QApplication a(argc, argv);

    qRegisterMetaType<Benchmark::Type>("Benchmark::Type");
    qRegisterMetaType<Benchmark::PerformanceResult>("Benchmark::PerfomanceResult");
    qRegisterMetaType<QMap<Benchmark::Type,QProgressBar*>>("QMap<Benchmark::Type,QProgressBar*>");

    QTranslator translator;
        if (translator.load(QLocale(), QLatin1String("KDiskMark"), QLatin1String("_")))
            a.installTranslator(&translator);

    MainWindow w;
    if (w.checkIfFIOInstalled()) {
        w.setFixedSize(531, 405);
        w.show();
        return a.exec();
    }
    else return -1;
}
