#include "mainwindow.h"

#include <QApplication>

#include "singleapplication.h"
#include "cmake.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral(PROJECT_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral("%1.%2.%3").arg(PROJECT_VERSION_MAJOR)
                                            .arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH));

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    SingleApplication a(argc, argv);

    AppSettings().setupLocalization();

    MainWindow w;
    w.setFixedSize(w.size());
    w.show();

    return a.exec();
}
