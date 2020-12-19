#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>
#include <QMessageBox>
#include <QLibraryInfo>
#include <QStyleFactory>
#include <QStandardPaths>

#include "cmake.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral(PROJECT_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral("%1.%2.%3").arg(PROJECT_VERSION_MAJOR)
                                            .arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH));

    QApplication a(argc, argv);

    a.setStyle(QStyleFactory::create("Fusion"));

    qRegisterMetaType<Benchmark::Type>("Benchmark::Type");
    qRegisterMetaType<Benchmark::PerformanceResult>("Benchmark::PerfomanceResult");
    qRegisterMetaType<QList<QPair<Benchmark::Type,QVector<QProgressBar*>>>>("QList<QPair<Benchmark::Type,QVector<QProgressBar*>>>");

    AppSettings settings;
    settings.setupLocalization();
    Benchmark benchmark(&settings);

    if (benchmark.isFIODetected()) {
        MainWindow w(&settings, &benchmark);
        w.setFixedSize(w.size());
        w.show();
        return a.exec();
    }
    else {
        QMessageBox::critical(0, "KDiskMark",
                              QObject::tr("No FIO was found. Please install FIO before using KDiskMark."));
        return -1;
    }
}
