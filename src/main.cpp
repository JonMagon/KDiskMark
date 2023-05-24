#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

#include "cmake.h"

#include "global.h"
#include "styletweaks.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral(PROJECT_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral("%1.%2.%3").arg(PROJECT_VERSION_MAJOR)
                                            .arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH));
    QCoreApplication::setOrganizationName(QStringLiteral(PROJECT_NAME));

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);

    AppSettings().setupLocalization();

    a.setStyle(new StyleTweaks());

#ifdef ROOT_EDITION
    // Override style when run as root
    if (Global::isRunningAsRoot()) {
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
