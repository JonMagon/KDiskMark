#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>

#include "cmake.h"

#include <unistd.h>

#ifdef ROOT_EDITION
#include "styletweaks.h"
#endif

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral(PROJECT_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral("%1.%2.%3").arg(PROJECT_VERSION_MAJOR)
                                            .arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH));

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);

    AppSettings().setupLocalization();

#ifdef ROOT_EDITION
    if (getuid() != 0) {
        QMessageBox::critical(0, "KDiskMark", "This edition of KDiskMark must be run as a root user.\n"
                                              "This can be done, for example, with the following command:\n"
                                              "sudo env DISPLAY=$DISPLAY XAUTHORITY=$XAUTHORITY kdiskmark");
        return -1;
    }

    a.setStyle(new StyleTweaks);

    QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << (qApp->applicationDirPath() + "/../share/icons/"));
    QIcon::setThemeName("breeze");
#else
    QMessageBox::warning(0, "KDiskMark", "This edition of KDiskMark has limitations that cannot be fixed.\n"
                                          "Clearing the cache and writing to protected directories will not be available.\n"
                                          "If possible, use the native package for the distribution or AppImage.");
#endif

    MainWindow w;
    w.setFixedSize(w.size());
    w.show();

    return a.exec();
}
