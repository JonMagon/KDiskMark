#include "mainwindow.h"

#include <QApplication>

#include <cstring>

#include "singleapplication.h"
#include "cmake.h"
#include "tui.h"

static bool isOptionPassed(int argc, char *argv[], const char *option)
{
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], option) == 0)
            return true;
    }

    return false;
}

// QApplication requires a graphical display. With --tui, --help, --version or
// without a display, the terminal interface is started over QCoreApplication.
static bool isTerminalRequested(int argc, char *argv[])
{
    for (const char *option : { "--tui", "-h", "--help", "--help-all", "-v", "--version" }) {
        if (isOptionPassed(argc, argv, option))
            return true;
    }

    return false;
}

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral(PROJECT_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral("%1.%2.%3").arg(PROJECT_VERSION_MAJOR)
                                            .arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH));
    QCoreApplication::setOrganizationName(QStringLiteral(PROJECT_NAME));

    const bool noDisplay = qEnvironmentVariableIsEmpty("DISPLAY")
                        && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");

    if (isTerminalRequested(argc, argv) || noDisplay) {
        QCoreApplication app(argc, argv);

        Tui tui;
        return tui.run(app);
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    SingleApplication a(argc, argv);

    AppSettings().setupLocalization();

    MainWindow w;
    w.setFixedSize(w.size());
    w.show();

    return a.exec();
}
