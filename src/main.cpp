#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

#include "cmake.h"

#ifdef ROOT_EDITION
#include <unistd.h>
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
    // Override style when run as root
    if (getuid() == 0) {
        StyleTweaks *styleTweaks = new StyleTweaks();
        styleTweaks->setBaseStyle(QStyleFactory::create("Breeze"));
        a.setStyle(styleTweaks);

        QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << (qApp->applicationDirPath() + "/../share/icons/"));
        QIcon::setThemeName("breeze");
    }
#endif

    MainWindow w;
    w.setFixedSize(w.size());
    w.show();

    return a.exec();
}
