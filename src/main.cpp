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

    AppSettings().setupLocalization();

    if (!Benchmark().isFIODetected()) {
        QMessageBox::critical(0, "KDiskMark",
                              QObject::tr("No FIO was found. Please install FIO before using KDiskMark."));
        return EXIT_FAILURE;
    }

    MainWindow w;
    w.setFixedSize(w.size());
    w.show();

    return a.exec();
}
