#include <QCoreApplication>
#include <QProcess>
#include <QDir>
#include <cstdio>
#include <windows.h>

static BOOL WINAPI ignoreConsoleControl(DWORD event)
{
    return event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QStringList arguments = app.arguments().mid(1);
    if (arguments.isEmpty() || !arguments.first().startsWith(QLatin1Char('-')))
        arguments.prepend(QStringLiteral("--tui"));
    SetConsoleCtrlHandler(ignoreConsoleControl, TRUE);
    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.setInputChannelMode(QProcess::ForwardedInputChannel);
    process.start(QDir(app.applicationDirPath()).filePath(QStringLiteral("kdiskmark.exe")), arguments);
    if (!process.waitForStarted()) { std::fputs("Cannot start kdiskmark.exe\n", stderr); return 2; }
    process.waitForFinished(-1);
    return process.exitStatus() == QProcess::NormalExit ? process.exitCode() : 2;
}
