#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>

#include "cmake.h"

#include <unistd.h>

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral(PROJECT_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral("%1.%2.%3").arg(PROJECT_VERSION_MAJOR)
                                            .arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH));

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);

    AppSettings().setupLocalization();

    if (getuid() != 0) {
        QMessageBox::critical(0, "KDiskMark", "This edition of KDiskMark must be run as a root user.\n"
                                              "This can be done, for example, with the following command:\n"
                                              "sudo env DISPLAY=$DISPLAY XAUTHORITY=$XAUTHORITY kdiskmark");
        return -1;
    }

    MainWindow w;
    w.setFixedSize(w.size());
    w.show();

    return a.exec();
}
