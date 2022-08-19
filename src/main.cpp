#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>
#include <QMessageBox>
#include <QLibraryInfo>
#include <QStandardPaths>

#include "cmake.h"

#include <KAuth>

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral(PROJECT_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral("%1.%2.%3").arg(PROJECT_VERSION_MAJOR)
                                            .arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH));
    
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    qRegisterMetaType<Benchmark::PerformanceResult>("Benchmark::PerfomanceResult");

    AppSettings().setupLocalization();

    //if (benchmark.isFIODetected()) {
        MainWindow w;
        w.setFixedSize(w.size());
        w.show();

        return a.exec();
    /*}
    else {
        QMessageBox::critical(0, "KDiskMark",
                              QObject::tr("No FIO was found. Please install FIO before using KDiskMark."));
        return -1;
    }*/
}
