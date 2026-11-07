#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

#include <cstring>
#ifdef Q_OS_WIN
#include <cstdio>
#endif

#include "cmake.h"
#include "global.h"
#include "styletweaks.h"
#include "tui.h"
#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <optional>
#include <string>
#endif

#ifdef APPIMAGE_EDITION
#include "helper.h"
#include <QDebug>
#endif

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

#ifdef Q_OS_WIN
static std::optional<int> elevateWindows(bool terminal)
{
    HANDLE token = nullptr;
    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    bool queried = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token);
    if (queried) {
        queried = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size);
        CloseHandle(token);
    }
    if (queried && elevation.TokenIsElevated)
        return std::nullopt;

    // Quote every native argument, preserving Unicode, quotes and trailing slashes.
    int count = 0;
    auto args = CommandLineToArgvW(GetCommandLineW(), &count);
    std::wstring parameters;
    if (!args)
        return 1;
    for (int i = 1; i < count; ++i) {
        parameters += L"\"";
        unsigned slashes = 0;
        for (const wchar_t *c = args[i]; *c; ++c) {
            if (*c == L'\\') { ++slashes; continue; }
            parameters.append(*c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
            parameters += *c;
            slashes = 0;
        }
        parameters.append(slashes * 2, L'\\');
        parameters += L"\" ";
    }
    LocalFree(args);
    wchar_t executable[32768];
    if (!GetModuleFileNameW(nullptr, executable, 32768))
        return 1;
    SHELLEXECUTEINFOW launch{};
    launch.cbSize = sizeof(launch);
    launch.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    launch.lpVerb = L"runas";
    launch.lpFile = executable;
    launch.lpParameters = parameters.c_str();
    launch.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&launch)) {
        const DWORD error = GetLastError();
        if (error != ERROR_CANCELLED) {
            wchar_t message[512]{};
            FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                           nullptr, error, 0, message, 512, nullptr);
            MessageBoxW(nullptr, message, L"KDiskMark", MB_OK | MB_ICONERROR);
        }
        return error == ERROR_CANCELLED ? ERROR_CANCELLED : 1;
    }
    DWORD code = 0;
    if (launch.hProcess) {
        if (terminal) {
            WaitForSingleObject(launch.hProcess, INFINITE);
            GetExitCodeProcess(launch.hProcess, &code);
        }
        CloseHandle(launch.hProcess);
    }
    return static_cast<int>(code);
}
#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    bool informationOnly = false;
    for (const char *option : { "-h", "--help", "--help-all", "-v", "--version" })
        informationOnly |= isOptionPassed(argc, argv, option);
    if (!informationOnly) {
        if (auto result = elevateWindows(isTerminalRequested(argc, argv)))
            return *result;
    }
#endif
#ifdef APPIMAGE_EDITION
    if (argc != 1) {
        if (QString::compare(argv[1], "--helper") == 0) {
            QCoreApplication a(argc, argv);
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

#ifdef Q_OS_WIN
    if (isTerminalRequested(argc, argv)) {
        if (!AttachConsole(ATTACH_PARENT_PROCESS) && !informationOnly && !GetConsoleWindow())
            AllocConsole();
        DWORD mode = 0;
        if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode))
            std::freopen("CONIN$", "r", stdin);
        if (GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode))
            std::freopen("CONOUT$", "w+", stdout);
        if (GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), &mode))
            std::freopen("CONOUT$", "w", stderr);
        QCoreApplication app(argc, argv);
        Tui tui;
        return tui.run(app);
    }
#else
    const bool noDisplay = qEnvironmentVariableIsEmpty("DISPLAY")
                        && qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");

    if (isTerminalRequested(argc, argv) || noDisplay) {
        QCoreApplication app(argc, argv);

        Tui tui;
        return tui.run(app);
    }
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication a(argc, argv);
#ifdef Q_OS_WIN
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/kdiskmark.ico")));
#else
    QApplication::setWindowIcon(QIcon(QStringLiteral("%1/../share/icons/hicolor/256x256/apps/%2.png")
                                      .arg(qApp->applicationDirPath()).arg(PROJECT_NAME)));
#endif

    AppSettings().setupLocalization();

    a.setStyle(new StyleTweaks());

    MainWindow w;
    w.setFixedSize(w.size());
    w.show();

    return a.exec();
}
