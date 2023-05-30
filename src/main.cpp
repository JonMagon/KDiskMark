#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

#include "cmake.h"

#include "global.h"
#include "styletweaks.h"

#ifdef APPIMAGE_EDITION
#include "helper.h"
#include <QDebug>
#endif

int main(int argc, char *argv[])
{
#ifdef APPIMAGE_EDITION
    if (argc != 1) {
        QCoreApplication a(argc, argv);
        if (QString::compare(a.arguments().at(1), "--helper") == 0) {
            if (Global::isRunningAsRoot()) {
                if (argc >= 3) {
                    Helper *helper = new Helper(a.arguments().at(2));
                    return helper->connectToServer() ? a.exec(): -1;
                }
                else {
                    qCritical() << "Helper id must be defined.";
                    return -1;
                }
            }
            else {
                qCritical() << "The helper must be run as superuser.";
                return -1;
            }
        }
    }

    if (Global::isRunningAsRoot()) {
        qCritical() << "You should run KDiskMark as normal user.";
        return -1;
    }
#endif
    QCoreApplication::setApplicationName(QStringLiteral(PROJECT_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral("%1.%2.%3").arg(PROJECT_VERSION_MAJOR)
                                            .arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH));
    QCoreApplication::setOrganizationName(QStringLiteral(PROJECT_NAME));

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    QApplication::setWindowIcon(QIcon(QStringLiteral("%1/../share/icons/hicolor/256x256/apps/%2.png")
                                      .arg(qApp->applicationDirPath()).arg(PROJECT_NAME)));

    AppSettings().setupLocalization();

    a.setStyle(new StyleTweaks());

    MainWindow w;
    w.setFixedSize(w.size());
    w.show();

    return a.exec();
}
