#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>
#include <QMessageBox>
#include "cmake.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral(PROJECT_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral("%1.%2.%3").arg(PROJECT_VERSION_MAJOR)
                                            .arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH));

    QApplication a(argc, argv);

    qRegisterMetaType<Benchmark::Type>("Benchmark::Type");
    qRegisterMetaType<Benchmark::PerformanceResult>("Benchmark::PerfomanceResult");
    qRegisterMetaType<QMap<Benchmark::Type,QProgressBar*>>("QMap<Benchmark::Type,QProgressBar*>");

    QTranslator translator;
    if (translator.load(QLocale(), qAppName(), QLatin1String("_"))) {
        a.installTranslator(&translator);
    }

    AppSettings settings;
    Benchmark benchmark(&settings);

    if (benchmark.isFIODetected()) {
        MainWindow w(&settings, &benchmark);
        w.setFixedSize(w.size());
        w.show();
        return a.exec();
    }
    else {
        QMessageBox::critical(0, qAppName(), "No FIO was found. Please install FIO before using KDiskMark.");
        return -1;
    }
}
