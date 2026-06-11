#include "tui.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QProcess>
#include <QSocketNotifier>
#include <QStorageInfo>
#include <QTextStream>
#include <QThread>
#include <QTime>

#include <cctype>
#include <climits>
#include <cmath>
#include <utility>

#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "appsettings.h"
#include "diskdriveinfo.h"
#include "global.h"

#include "helper_interface.h"

namespace {

enum SpecialKey {
    KeyUp = 0x1000,
    KeyDown,
    KeyRight,
    KeyLeft,
    KeyEscape
};

// The number of terminal cells the text occupies: East Asian wide characters
// take two cells, combining marks take none. QString::leftJustified() counts
// code units and misaligns such texts.
int displayWidth(const QString &text)
{
    int width = 0;

    const auto ucs4 = text.toUcs4();
    for (uint ch : ucs4) {
        const QChar::Category category = QChar::category(ch);
        if (category == QChar::Mark_NonSpacing || category == QChar::Mark_Enclosing || ch == 0x200B)
            continue;

        if ((ch >= 0x1100 && ch <= 0x115F)     // Hangul Jamo
                || (ch >= 0x2E80 && ch <= 0x303E)   // CJK radicals, ideographic punctuation
                || (ch >= 0x3041 && ch <= 0x33FF)   // Kana, CJK symbols
                || (ch >= 0x3400 && ch <= 0x4DBF)   // CJK Extension A
                || (ch >= 0x4E00 && ch <= 0x9FFF)   // CJK Unified Ideographs
                || (ch >= 0xA000 && ch <= 0xA4CF)   // Yi
                || (ch >= 0xA960 && ch <= 0xA97F)   // Hangul Jamo Extended-A
                || (ch >= 0xAC00 && ch <= 0xD7A3)   // Hangul Syllables
                || (ch >= 0xF900 && ch <= 0xFAFF)   // CJK Compatibility Ideographs
                || (ch >= 0xFE30 && ch <= 0xFE4F)   // CJK Compatibility Forms
                || (ch >= 0xFF00 && ch <= 0xFF60)   // Fullwidth Forms
                || (ch >= 0xFFE0 && ch <= 0xFFE6)
                || (ch >= 0x1F300 && ch <= 0x1F64F) // Emoji
                || (ch >= 0x20000 && ch <= 0x3FFFD))
            width += 2;
        else
            width += 1;
    }

    return width;
}

QString padDisplay(const QString &text, int width) // left-justified
{
    const int padding = width - displayWidth(text);
    return padding > 0 ? text + QString(padding, QLatin1Char(' ')) : text;
}

QString padDisplayLeft(const QString &text, int width) // right-justified
{
    const int padding = width - displayWidth(text);
    return padding > 0 ? QString(padding, QLatin1Char(' ')) + text : text;
}

int signalSocketPair[2] = { -1, -1 };

void unixSignalHandler(int signum)
{
    const char byte = (char)signum;
    if (::write(signalSocketPair[1], &byte, sizeof(byte)) != sizeof(byte)) {
        // Cannot do anything async-signal-safe about it
    }
}

} // namespace

const QVector<int> Tui::s_fileSizes = { 16, 32, 64, 128, 256, 512,
                                        1024, 2048, 4096, 8192, 16384, 32768, 65536 };

// The same value sets as in the GUI settings dialog
const QVector<int> Tui::s_blockSizes = { 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192 };
const QVector<int> Tui::s_queues = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 };
const QVector<int> Tui::s_measureTimes = { 5, 10, 20, 30, 60 };
const QVector<int> Tui::s_intervalTimes = { 0, 1, 3, 5, 10, 30, 60, 180, 300, 600 };

Tui::Tui(QObject *parent)
    : QObject(parent)
{
}

Tui::~Tui()
{
    leaveScreen();
}

int Tui::run(QCoreApplication &app)
{
    AppSettings().setupLocalization();

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("\n") +
        tr("A simple open source disk benchmark tool for Linux distros."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption(QStringLiteral("tui"),
        tr("Run the interactive terminal interface (default when no graphical display is available).")));
    parser.addPositionalArgument(QStringLiteral("directory"),
                                 tr("Directory to add to the storage list."),
                                 QStringLiteral("[directory]"));
    parser.process(app);

    QTextStream err(stderr);

    if (!::isatty(STDIN_FILENO) || !::isatty(STDOUT_FILENO)) {
        err << tr("The terminal interface requires an interactive terminal.") << "\n";
        return 2;
    }

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.size() > 1) {
        err << tr("Only one directory can be specified.") << "\n";
        return 2;
    }

    QString customDir = positionalArguments.value(0);
    if (!customDir.isEmpty()) {
        const QFileInfo dirInfo(customDir);
        if (!dirInfo.isDir()) {
            err << tr("Directory does not exist: %1").arg(customDir) << "\n";
            return 2;
        }
        customDir = dirInfo.absoluteFilePath();
    }

    m_benchmark = new Benchmark;
    m_benchmark->setParent(this);

    if (!m_benchmark->isFIODetected()) {
        err << tr("No FIO was found. Please install FIO before using KDiskMark.") << "\n";
        return 2;
    }

    connect(m_benchmark, &Benchmark::benchmarkStatusUpdate, this, &Tui::benchmarkStatusUpdate);
    connect(m_benchmark, &Benchmark::resultReady, this, &Tui::handleResults);
    connect(m_benchmark, &Benchmark::failed, this, &Tui::benchmarkFailed);
    connect(m_benchmark, &Benchmark::cowCheckRequired, this, &Tui::handleCowCheck);
    connect(m_benchmark, &Benchmark::directoryChanged, this, &Tui::handleDirectoryChanged);

    setupUnixSignalHandlers();

    m_unitIndex = (int)AppSettings().getComparisonUnit();

    rescanStorages();
    if (!customDir.isEmpty())
        addCustomStorage(customDir, true);
    else if (!m_storages.isEmpty())
        selectStorage(0);

    rebuildRows();

    m_keyNotifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
    connect(m_keyNotifier, &QSocketNotifier::activated, this, &Tui::handleKeyInput);
    m_keyNotifier->setEnabled(false);

    m_status = tr("Ready.");

    enterScreen();
    render();

    const int code = app.exec();

    leaveScreen();

    return code;
}

void Tui::enterScreen()
{
    if (m_screenActive)
        return;

    ::tcgetattr(STDIN_FILENO, &m_originalTermios);
    struct termios raw = m_originalTermios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // Alternate screen buffer, hidden cursor, no line wrapping
    QTextStream out(stdout);
    out << QStringLiteral("\x1b[?1049h\x1b[?25l\x1b[?7l\x1b[2J\x1b[H");
    out.flush();

    m_screenActive = true;
    if (m_keyNotifier)
        m_keyNotifier->setEnabled(true);

    updateTerminalSize();
}

void Tui::leaveScreen()
{
    if (!m_screenActive)
        return;

    if (m_keyNotifier)
        m_keyNotifier->setEnabled(false);

    QTextStream out(stdout);
    out << QStringLiteral("\x1b[?7h\x1b[?25h\x1b[?1049l");
    out.flush();

    ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &m_originalTermios);

    m_screenActive = false;
}

void Tui::updateTerminalSize()
{
    struct winsize size = {};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0 && size.ws_row > 0) {
        m_termWidth = size.ws_col;
        m_termHeight = size.ws_row;
    }
    else {
        m_termWidth = 80;
        m_termHeight = 24;
    }
}

QString Tui::promptLine(const QString &prompt)
{
    if (m_keyNotifier)
        m_keyNotifier->setEnabled(false);

    QTextStream out(stdout);
    out << QStringLiteral("\x1b[%1;1H\x1b[K").arg(m_termHeight) << prompt << QStringLiteral("\x1b[?25h");
    out.flush();

    // The kernel provides line editing in the canonical mode
    struct termios raw = {};
    ::tcgetattr(STDIN_FILENO, &raw);
    struct termios cooked = raw;
    cooked.c_lflag |= (ICANON | ECHO);
    ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);

    QByteArray line;
    char ch = 0;
    while (::read(STDIN_FILENO, &ch, 1) == 1 && ch != '\n')
        line += ch;

    ::tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    out << QStringLiteral("\x1b[?25l");
    out.flush();

    if (m_keyNotifier)
        m_keyNotifier->setEnabled(true);

    return QString::fromUtf8(line).trimmed();
}

void Tui::handleKeyInput()
{
    char buffer[64];
    const ssize_t bytesRead = ::read(STDIN_FILENO, buffer, sizeof(buffer));
    if (bytesRead <= 0)
        return;

    int i = 0;
    while (i < bytesRead) {
        int key = 0;

        if (buffer[i] == '\x1b') {
            if (i + 1 < bytesRead && (buffer[i + 1] == '[' || buffer[i + 1] == 'O')) {
                int j = i + 2;
                while (j < bytesRead && !::isalpha((unsigned char)buffer[j]) && buffer[j] != '~')
                    j++;
                const char finalByte = j < bytesRead ? buffer[j] : 0;
                i = j + 1;

                switch (finalByte) {
                case 'A': key = KeyUp; break;
                case 'B': key = KeyDown; break;
                case 'C': key = KeyRight; break;
                case 'D': key = KeyLeft; break;
                default: continue;
                }
            }
            else { // A lone Escape
                key = KeyEscape;
                i++;
            }
        }
        else {
            key = (unsigned char)buffer[i];
            i++;
        }

        processKey(key);
    }
}

void Tui::processKey(int key)
{
    if (m_benchmark->isRunning()) {
        // While benchmarking only stopping is available
        if (key == '\r' || key == '\n' || key == ' ')
            stopOrQuit(false);
        else if (key == 'q' || key == 'Q')
            stopOrQuit(true);
        return;
    }

    if (m_screen == SettingsScreen) {
        const int count = m_settingsItems.size();

        switch (key) {
        case KeyUp:
        case 'k':
            m_settingsFocus = (m_settingsFocus - 1 + count) % count;
            render();
            break;
        case KeyDown:
        case '\t':
        case 'j':
            m_settingsFocus = (m_settingsFocus + 1) % count;
            render();
            break;
        case KeyRight:
        case 'l':
            changeSettingsValue(m_settingsItems.at(m_settingsFocus), 1);
            break;
        case KeyLeft:
        case 'h':
            changeSettingsValue(m_settingsItems.at(m_settingsFocus), -1);
            break;
        case '\r':
        case '\n':
        case ' ':
            activateSettingsItem();
            break;
        case KeyEscape:
        case 'q':
        case 'Q':
        case 'e':
        case 'E':
            m_screen = MainScreen;
            render();
            break;
        default:
            break;
        }

        return;
    }

    const QVector<Field> fields = visibleFields();
    const bool onField = m_focus < fields.size();

    switch (key) {
    case KeyUp:
    case 'k':
        moveFocus(-1);
        break;
    case KeyDown:
    case '\t':
    case 'j':
        moveFocus(1);
        break;
    case KeyRight:
    case 'l':
        if (onField)
            changeFieldValue(fields.at(m_focus), 1);
        break;
    case KeyLeft:
    case 'h':
        if (onField)
            changeFieldValue(fields.at(m_focus), -1);
        break;
    case '\r':
    case '\n':
        activateFocused(true);
        break;
    case ' ':
        activateFocused(false);
        break;
    case 'e':
    case 'E':
        openSettingsScreen();
        break;
    case 's':
    case 'S':
        saveReportInteractive();
        break;
    case 'r':
    case 'R':
        rescanStorages();
        render();
        break;
    case 'q':
    case 'Q':
        stopOrQuit(true);
        break;
    default:
        break;
    }
}

QVector<Tui::Field> Tui::visibleFields() const
{
    QVector<Field> fields { FieldStart, FieldProfile, FieldSize, FieldLoops, FieldStorage, FieldUnit };
    if (isMixedActive())
        fields << FieldMixRatio;
    fields << FieldLanguage;
    return fields;
}

int Tui::focusItemsCount() const
{
    return visibleFields().size() + m_rows.size();
}

void Tui::moveFocus(int delta)
{
    const int count = focusItemsCount();
    if (count <= 0)
        return;

    m_focus = (m_focus + delta % count + count) % count;
    render();
}

void Tui::activateFocused(bool enterKey)
{
    const QVector<Field> fields = visibleFields();

    if (m_focus < fields.size()) {
        const Field field = fields.at(m_focus);
        if (field == FieldStart)
            startBenchmark(-1);
        else if (field == FieldStorage && enterKey)
            promptForDirectory();
        else
            changeFieldValue(field, 1);
    }
    else {
        const int row = m_focus - fields.size();
        if (row >= 0 && row < m_rows.size())
            startBenchmark(row);
    }
}

void Tui::changeFieldValue(Field field, int direction)
{
    AppSettings settings;

    switch (field) {
    case FieldStart:
        return;

    case FieldProfile: {
        // Profile and mixed combinations, as in the GUI menu
        struct ProfileOption { Global::PerformanceProfile profile; bool mixed; };
        static const QVector<ProfileOption> options = {
            { Global::PerformanceProfile::Default,   false },
            { Global::PerformanceProfile::Default,   true  },
            { Global::PerformanceProfile::Peak,      false },
            { Global::PerformanceProfile::Peak,      true  },
            { Global::PerformanceProfile::RealWorld, false },
            { Global::PerformanceProfile::RealWorld, true  },
            { Global::PerformanceProfile::Demo,      false },
        };

        int index = 0;
        for (int i = 0; i < options.size(); i++) {
            if (options.at(i).profile == settings.getPerformanceProfile()
                    && options.at(i).mixed == isMixedActive()) {
                index = i;
                break;
            }
        }

        index = (index + direction + options.size()) % options.size();
        settings.setPerformanceProfile(options.at(index).profile);
        settings.setMixedState(options.at(index).mixed);

        rebuildRows();
        break;
    }

    case FieldSize: {
        const int fileSize = settings.getFileSize();
        int index = 0;
        int bestDifference = INT_MAX;
        for (int i = 0; i < s_fileSizes.size(); i++) {
            const int difference = qAbs(s_fileSizes.at(i) - fileSize);
            if (difference < bestDifference) {
                bestDifference = difference;
                index = i;
            }
        }
        index = qBound(0, index + direction, s_fileSizes.size() - 1);
        settings.setFileSize(s_fileSizes.at(index));
        break;
    }

    case FieldLoops:
        settings.setLoopsCount(qBound(1, settings.getLoopsCount() + direction, 9));
        break;

    case FieldStorage:
        if (!m_storages.isEmpty())
            selectStorage((m_storageIndex + direction + m_storages.size()) % m_storages.size());
        break;

    case FieldUnit:
        m_unitIndex = (m_unitIndex + direction + 4) % 4;
        settings.setComparisonUnit((Global::ComparisonUnit)m_unitIndex);
        break;

    case FieldMixRatio:
        settings.setRandomReadPercentage(qBound(10, settings.getRandomReadPercentage() + direction * 10, 90));
        break;

    case FieldLanguage: {
        const QVector<QLocale> locales = Global::getSupportedLocales();
        int index = 0;
        for (int i = 0; i < locales.size(); i++) {
            if (locales.at(i).name() == QLocale().name()) {
                index = i;
                break;
            }
        }

        index = (index + direction + locales.size()) % locales.size();
        settings.setLocale(locales.at(index));

        // Already displayed texts do not retranslate themselves
        m_status = tr("Ready.");
        m_lastError.clear();
        m_infoMessage.clear();
        break;
    }
    }

    if (m_focus >= focusItemsCount())
        m_focus = focusItemsCount() - 1;

    render();
}

void Tui::promptForDirectory()
{
    const QString path = promptLine(tr("Directory: "));

    if (!path.isEmpty()) {
        const QFileInfo dirInfo(path);
        if (dirInfo.isDir()) {
            addCustomStorage(dirInfo.absoluteFilePath(), true);
            m_lastError.clear();
        }
        else {
            m_lastError = tr("Directory does not exist: %1").arg(path);
        }
    }

    render();
}

void Tui::openSettingsScreen()
{
    buildSettingsItems();
    m_settingsFocus = 0;
    m_screen = SettingsScreen;
    render();
}

void Tui::buildSettingsItems()
{
    m_settingsItems.clear();

    const QVector<Global::BenchmarkTest> tests = editableTests(AppSettings().getPerformanceProfile());
    for (Global::BenchmarkTest test : tests) {
        m_settingsItems << SettingsItem { SettingsItem::TestPattern, test }
                        << SettingsItem { SettingsItem::TestBlockSize, test }
                        << SettingsItem { SettingsItem::TestQueues, test }
                        << SettingsItem { SettingsItem::TestThreads, test };
    }

    m_settingsItems << SettingsItem { SettingsItem::MeasureTime }
                    << SettingsItem { SettingsItem::IntervalTime }
                    << SettingsItem { SettingsItem::Mode }
                    << SettingsItem { SettingsItem::TestData }
                    << SettingsItem { SettingsItem::Continuous }
                    << SettingsItem { SettingsItem::FlushCache }
                    << SettingsItem { SettingsItem::CacheBypass }
                    << SettingsItem { SettingsItem::CowDetection }
                    << SettingsItem { SettingsItem::PresetStandard }
                    << SettingsItem { SettingsItem::PresetNvme }
                    << SettingsItem { SettingsItem::Back };
}

void Tui::changeSettingsValue(const SettingsItem &item, int direction)
{
    AppSettings settings;
    const Global::PerformanceProfile profile = settings.getPerformanceProfile();

    auto listStep = [direction](const QVector<int> &values, int current) {
        int index = 0;
        int bestDifference = INT_MAX;
        for (int i = 0; i < values.size(); i++) {
            const int difference = qAbs(values.at(i) - current);
            if (difference < bestDifference) {
                bestDifference = difference;
                index = i;
            }
        }
        return values.at(qBound(0, index + direction, values.size() - 1));
    };

    switch (item.kind) {
    case SettingsItem::TestPattern:
    case SettingsItem::TestBlockSize:
    case SettingsItem::TestQueues:
    case SettingsItem::TestThreads: {
        Global::BenchmarkParams params = settings.getBenchmarkParams(item.test, profile);

        if (item.kind == SettingsItem::TestPattern)
            params.Pattern = params.Pattern == Global::BenchmarkIOPattern::SEQ ? Global::BenchmarkIOPattern::RND
                                                                               : Global::BenchmarkIOPattern::SEQ;
        else if (item.kind == SettingsItem::TestBlockSize)
            params.BlockSize = listStep(s_blockSizes, params.BlockSize);
        else if (item.kind == SettingsItem::TestQueues)
            params.Queues = listStep(s_queues, params.Queues);
        else
            params.Threads = qBound(1, params.Threads + direction, 64);

        settings.setBenchmarkParams(item.test, profile, params);
        break;
    }

    case SettingsItem::MeasureTime:
        settings.setMeasuringTime(listStep(s_measureTimes, settings.getMeasuringTime()));
        break;

    case SettingsItem::IntervalTime:
        settings.setIntervalTime(listStep(s_intervalTimes, settings.getIntervalTime()));
        break;

    case SettingsItem::Mode: {
        const int mode = qBound(0, (int)settings.getBenchmarkMode() + direction, 2);
        settings.setBenchmarkMode((Global::BenchmarkMode)mode);
        break;
    }

    case SettingsItem::TestData:
        settings.setBenchmarkTestData(settings.getBenchmarkTestData() == Global::BenchmarkTestData::Random
                                      ? Global::BenchmarkTestData::Zeros : Global::BenchmarkTestData::Random);
        break;

    case SettingsItem::Continuous:
        settings.setContinuousGenerationState(!settings.getContinuousGenerationState());
        break;

    case SettingsItem::FlushCache:
        settings.setFlushingCacheState(!settings.getFlusingCacheState());
        break;

    case SettingsItem::CacheBypass:
        settings.setCacheBypassState(!settings.getCacheBypassState());
        break;

    case SettingsItem::CowDetection:
        settings.setCoWDetectionState(!settings.getCoWDetectionState());
        break;

    case SettingsItem::PresetStandard:
    case SettingsItem::PresetNvme:
    case SettingsItem::Back:
        return;
    }

    render();
}

void Tui::activateSettingsItem()
{
    const SettingsItem item = m_settingsItems.at(m_settingsFocus);

    switch (item.kind) {
    case SettingsItem::PresetStandard:
        applyPresetToSettings(Global::BenchmarkPreset::Standard);
        m_infoMessage = tr("Standard preset applied.");
        render();
        break;
    case SettingsItem::PresetNvme:
        applyPresetToSettings(Global::BenchmarkPreset::NVMe_SSD);
        m_infoMessage = tr("NVMe SSD preset applied.");
        render();
        break;
    case SettingsItem::Back:
        m_screen = MainScreen;
        render();
        break;
    default:
        changeSettingsValue(item, 1);
        break;
    }
}

void Tui::applyPresetToSettings(Global::BenchmarkPreset preset)
{
    AppSettings settings;

    // The same scope as the GUI settings dialog presets
    for (const auto &profile : { Global::PerformanceProfile::Default,
                                 Global::PerformanceProfile::Peak,
                                 Global::PerformanceProfile::Demo }) {
        for (Global::BenchmarkTest test : editableTests(profile))
            settings.setBenchmarkParams(test, profile,
                AppSettings::defaultBenchmarkParams(test, profile, preset));
    }

    settings.setMeasuringTime(AppSettings::defaultMeasuringTime());
    settings.setIntervalTime(AppSettings::defaultIntervalTime());
}

QString Tui::timeText(int seconds) const
{
    return seconds < 60 ? QStringLiteral("%1 %2").arg(seconds).arg(tr("sec"))
                        : QStringLiteral("%1 %2").arg(seconds / 60).arg(tr("min"));
}

void Tui::startBenchmark(int onlyRow)
{
    if (m_benchmark->isRunning())
        return;

    if (m_storageIndex < 0) {
        m_lastError = tr("Directory is not specified.");
        render();
        return;
    }

    m_lastError.clear();
    m_infoMessage.clear();
    m_stopIssued = false;

    // The polkit agent prompt must not interfere with the alternate screen
    // and the raw input, so the terminal is released while authorizing.
    QString authorizationError;
    bool authorized;
    if (::geteuid() != 0) {
        leaveScreen();

        QTextStream out(stdout);
        out << tr("Requesting administrator privileges...") << "\n";
        out.flush();

        authorized = authorize(&authorizationError);
        enterScreen();
    }
    else {
        authorized = authorize(&authorizationError);
    }

    if (!authorized) {
        m_lastError = authorizationError.replace(QLatin1Char('\n'), QLatin1Char(' '));
        render();
        return;
    }

    buildTestPlan(onlyRow);

    m_status = tr("Preparing...");
    render();

    // Blocking: the interface stays live through the nested event loops
    m_benchmark->runBenchmark(m_testPlan);

    m_stopRequested = false;

    if (m_quitRequested) {
        leaveScreen();
        QCoreApplication::exit(m_exitCode);
        return;
    }

    if (m_lastError.isEmpty())
        m_status = m_stopIssued ? tr("Stopped.") : tr("Finished. Press S to save the report.");

    render();
}

void Tui::stopOrQuit(bool quit)
{
    if (quit)
        m_quitRequested = true;

    if (m_benchmark->isRunning()) {
        m_stopIssued = true;
        m_status = tr("Stopping...");
        render();
        m_benchmark->setRunning(false);
    }
    else if (m_quitRequested) {
        leaveScreen();
        QCoreApplication::exit(m_exitCode);
    }
}

void Tui::rescanStorages(const QString &preferredPath)
{
    QString currentPath = preferredPath;
    if (currentPath.isEmpty() && m_storageIndex >= 0 && m_storageIndex < m_storages.size())
        currentPath = m_storages.at(m_storageIndex).storage.path;

    QVector<StorageEntry> permanentEntries;
    for (const StorageEntry &entry : std::as_const(m_storages)) {
        if (entry.storage.permanentInList)
            permanentEntries << entry;
    }

    m_storages.clear();

    foreach (const QStorageInfo &volume, QStorageInfo::mountedVolumes()) {
        if (volume.isValid() && volume.isReady() && !volume.isReadOnly()
                && volume.device().indexOf("/dev") != -1) {
            StorageEntry entry;
            entry.storage = Global::Storage {
                volume.rootPath(),
                volume.bytesTotal(),
                volume.bytesTotal() - volume.bytesFree(),
                QString()
            };
            entry.modelName = DiskDriveInfo::Instance().getModelName(volume.device());
            entry.encrypted = DiskDriveInfo::Instance().isEncrypted(volume.device());
            m_storages << entry;
        }
    }

    for (const StorageEntry &permanentEntry : std::as_const(permanentEntries)) {
        bool exists = false;
        for (const StorageEntry &entry : std::as_const(m_storages)) {
            if (entry.storage.path == permanentEntry.storage.path) {
                exists = true;
                break;
            }
        }
        if (!exists)
            m_storages << permanentEntry;
    }

    m_storageIndex = -1;

    int restoredIndex = 0;
    if (!currentPath.isEmpty()) {
        for (int i = 0; i < m_storages.size(); i++) {
            if (m_storages.at(i).storage.path == currentPath) {
                restoredIndex = i;
                break;
            }
        }
    }

    if (!m_storages.isEmpty())
        selectStorage(restoredIndex);
}

void Tui::selectStorage(int index)
{
    if (index < 0 || index >= m_storages.size()) {
        m_storageIndex = -1;
        return;
    }

    m_storageIndex = index;
    m_benchmark->setDir(m_storages.at(index).storage.path);
}

void Tui::addCustomStorage(const QString &path, bool select)
{
    for (int i = 0; i < m_storages.size(); i++) {
        if (m_storages.at(i).storage.path == path) {
            if (select)
                selectStorage(i);
            return;
        }
    }

    const QStorageInfo volume(path);

    StorageEntry entry;
    entry.storage = Global::Storage {
        path,
        volume.bytesTotal(),
        volume.bytesTotal() - volume.bytesFree(),
        QString(),
        true
    };
    entry.modelName = DiskDriveInfo::Instance().getModelName(volume.device());
    entry.encrypted = DiskDriveInfo::Instance().isEncrypted(volume.device());

    m_storages << entry;

    if (select)
        selectStorage(m_storages.size() - 1);
}

bool Tui::isMixedActive()
{
    const AppSettings settings;
    return settings.getMixedState()
            && settings.getPerformanceProfile() != Global::PerformanceProfile::Demo;
}

QVector<Global::BenchmarkTest> Tui::profileTests(Global::PerformanceProfile profile)
{
    switch (profile) {
    case Global::PerformanceProfile::Default:
        return { Global::BenchmarkTest::Test_1, Global::BenchmarkTest::Test_2,
                 Global::BenchmarkTest::Test_3, Global::BenchmarkTest::Test_4 };
    case Global::PerformanceProfile::Peak:
    case Global::PerformanceProfile::RealWorld:
        return { Global::BenchmarkTest::Test_1, Global::BenchmarkTest::Test_2 };
    case Global::PerformanceProfile::Demo:
        return { Global::BenchmarkTest::Test_1 };
    }
    return {};
}

QVector<Global::BenchmarkTest> Tui::editableTests(Global::PerformanceProfile profile)
{
    // The same editable set as the GUI settings dialog: the Real World
    // profile uses fixed parameters
    switch (profile) {
    case Global::PerformanceProfile::Default:
        return { Global::BenchmarkTest::Test_1, Global::BenchmarkTest::Test_2,
                 Global::BenchmarkTest::Test_3, Global::BenchmarkTest::Test_4 };
    case Global::PerformanceProfile::Peak:
        return { Global::BenchmarkTest::Test_1, Global::BenchmarkTest::Test_2 };
    case Global::PerformanceProfile::Demo:
        return { Global::BenchmarkTest::Test_1 };
    case Global::PerformanceProfile::RealWorld:
        break;
    }
    return {};
}

void Tui::rebuildRows()
{
    m_rows.clear();

    const QVector<Global::BenchmarkTest> tests = profileTests(AppSettings().getPerformanceProfile());
    for (Global::BenchmarkTest test : tests)
        m_rows.append(Row { test, {} });
}

void Tui::buildTestPlan(int onlyRow)
{
    m_testPlan.clear();

    const QList<QObject*> oldCells = m_cells.keys();
    for (QObject *cell : oldCells)
        cell->deleteLater();
    m_cells.clear();

    auto cell = [this](int row, Column column) {
        QObject *target = new QObject(this);
        m_cells.insert(target, { row, column });
        return target;
    };

    auto addPass = [&](Global::BenchmarkIOReadWrite io, Column column) {
        for (int i = 0; i < m_rows.size(); i++) {
            if (onlyRow != -1 && i != onlyRow)
                continue;
            m_testPlan << qMakePair(qMakePair(m_rows.at(i).test, io),
                                    QVector<QObject*> { cell(i, column) });
        }
    };

    addPass(Global::BenchmarkIOReadWrite::Read, ColRead);
    addPass(Global::BenchmarkIOReadWrite::Write, ColWrite);
    if (isMixedActive())
        addPass(Global::BenchmarkIOReadWrite::Mix, ColMix);
}

bool Tui::storeResult(QObject *target, const Benchmark::PerformanceResult &result)
{
    const auto cell = m_cells.constFind(target);
    if (cell == m_cells.constEnd())
        return false;

    m_rows[cell->first].results[cell->second] = result;
    return true;
}

void Tui::startPolkitTtyAgent()
{
    if (m_polkitAgent)
        return;

    // Register a fallback text authentication agent, so that authentication
    // is also possible without a graphical polkit agent (e.g. on a virtual console).
    m_polkitAgent = new QProcess(this);
    m_polkitAgent->setProcessChannelMode(QProcess::ForwardedChannels);
    m_polkitAgent->setInputChannelMode(QProcess::ForwardedInputChannel);
    m_polkitAgent->start(QStringLiteral("pkttyagent"),
                         { QStringLiteral("--fallback"),
                           QStringLiteral("--process"),
                           QString::number(QCoreApplication::applicationPid()) });

    if (m_polkitAgent->waitForStarted(3000))
        QThread::msleep(500); // Give the agent a moment to register with polkit
}

bool Tui::authorize(QString *errorMessage)
{
    QString localError;
    bool accessDenied = false;

    if (!QDBusConnection::systemBus().isConnected()) {
        *errorMessage = QDBusConnection::systemBus().lastError().message();
        return false;
    }

    auto attempt = [this, &localError, &accessDenied]() {
        localError.clear();
        accessDenied = false;

        dev::jonmagon::kdiskmark::helper interface(QStringLiteral("dev.jonmagon.kdiskmark.helperinterface"),
                                                   QStringLiteral("/Helper"), QDBusConnection::systemBus());
        interface.setTimeout(10 * 60 * 1000);

        QDBusPendingCall pcall = interface.initSession();
        QDBusPendingCallWatcher watcher(pcall);
        QEventLoop loop;
        connect(&watcher, &QDBusPendingCallWatcher::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (watcher.isError()) {
            accessDenied = watcher.error().type() == QDBusError::AccessDenied;
            localError = !watcher.error().message().isEmpty() ? watcher.error().message()
                                                              : watcher.error().name();
            return false;
        }

        const QDBusPendingReply<QVariantMap> reply = pcall;
        const QVariantMap response = reply.value();
        if (!response[QStringLiteral("success")].toBool()) {
            localError = response[QStringLiteral("error")].toString();
            return false;
        }

        return true;
    };

    bool authorized = attempt();

    // Authorization may have failed because no polkit agent is available.
    // Register the text fallback agent and retry once.
    if (!authorized && accessDenied && ::geteuid() != 0) {
        startPolkitTtyAgent();
        authorized = attempt();
    }

    if (authorized)
        return true;

    if (accessDenied) {
        localError = tr("Could not obtain administrator privileges.") + QStringLiteral(" ")
                   + tr("Run the application as root or make sure a polkit authentication agent is available.");
    }
    else if (localError.isEmpty()) {
        localError = tr("Could not communicate with the helper.");
    }

    *errorMessage = localError;

    return false;
}

void Tui::setupUnixSignalHandlers()
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, signalSocketPair) != 0)
        return;

    m_signalNotifier = new QSocketNotifier(signalSocketPair[0], QSocketNotifier::Read, this);
    connect(m_signalNotifier, &QSocketNotifier::activated, this, &Tui::handleUnixSignal);

    struct sigaction action = {};
    action.sa_handler = unixSignalHandler;
    ::sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    ::sigaction(SIGINT, &action, nullptr);
    ::sigaction(SIGTERM, &action, nullptr);
    ::sigaction(SIGHUP, &action, nullptr);
    ::sigaction(SIGWINCH, &action, nullptr);
}

void Tui::handleUnixSignal()
{
    char signum = 0;
    if (::read(signalSocketPair[0], &signum, sizeof(signum)) != sizeof(signum)) {
        // Spurious wakeup
    }

    if (signum == SIGWINCH) {
        updateTerminalSize();
        render();
        return;
    }

    if (m_stopRequested) { // The second signal exits immediately
        leaveScreen();
        ::_exit(130);
    }

    m_stopRequested = true;
    m_exitCode = 130;
    stopOrQuit(true);
}

void Tui::benchmarkStatusUpdate(const QString &name)
{
    m_status = name;
    render();
}

void Tui::handleResults(QObject *target, const Benchmark::PerformanceResult &result)
{
    if (storeResult(target, result))
        render();
}

void Tui::benchmarkFailed(const QString &error)
{
    m_lastError = QString(error).simplified();
    render();
}

void Tui::handleCowCheck()
{
    bool create = false;

    if (!m_stopRequested && !m_quitRequested) {
        leaveScreen();

        QTextStream out(stdout);
        out << tr("Copy-on-Write (CoW) is enabled on the selected directory.") << "\n";
        out << tr("This may affect performance results. Would you like to create a new subdirectory with CoW disabled?")
            << " " << tr("[Y/n]") << " ";
        out.flush();

        // Read the cooked-mode line over the raw descriptor, so that no
        // type-ahead ends up in a FILE buffer
        QByteArray line;
        char ch = 0;
        while (::read(STDIN_FILENO, &ch, 1) == 1 && ch != '\n')
            line += ch;

        const QString answer = QString::fromUtf8(line).trimmed().toLower();
        create = answer.isEmpty() || answer == QStringLiteral("y") || answer == QStringLiteral("yes");

        enterScreen();
        render();
    }

    emit m_benchmark->createNoCowDirectoryResponse(create);
}

void Tui::handleDirectoryChanged(const QString &newDir)
{
    addCustomStorage(newDir, true);
    render();
}

void Tui::saveReportInteractive()
{
    const QString fileName = QStringLiteral("KDM_%1%2.txt")
            .arg(QDate::currentDate().toString(QStringLiteral("yyyyMMdd")),
                 QTime::currentTime().toString(QStringLiteral("hhmmss")));

    const QString filePath = QDir::current().absoluteFilePath(fileName);

    if (saveReport(filePath)) {
        m_infoMessage = tr("Report saved: %1").arg(filePath);
        m_lastError.clear();
    }
    else {
        m_lastError = tr("Failed to save the report to %1").arg(filePath);
    }

    render();
}

QString Tui::buildReport() const
{
    const AppSettings settings;
    const Global::PerformanceProfile profile = settings.getPerformanceProfile();
    const bool mixed = isMixedActive();

    QStringList output;

    output << QStringLiteral("KDiskMark (%1): https://github.com/JonMagon/KDiskMark")
              .arg(QCoreApplication::applicationVersion())
              .rightJustified(Global::getOutputColumnsCount(), ' ')
           << QStringLiteral("Flexible I/O Tester (%1): https://github.com/axboe/fio")
              .arg(m_benchmark->getFIOVersion())
              .rightJustified(Global::getOutputColumnsCount(), ' ')
           << QStringLiteral("-").repeated(Global::getOutputColumnsCount())
           << QStringLiteral("* MB/s = 1,000,000 bytes/s [SATA/600 = 600,000,000 bytes/s]")
           << QStringLiteral("* KB = 1000 bytes, KiB = 1024 bytes");

    auto appendColumn = [this, &output, &settings, profile](int column) {
        for (const Row &row : std::as_const(m_rows))
            output << reportTestLine(settings.getBenchmarkParams(row.test, profile), row.results[column]);
    };

    output << QString()
           << QStringLiteral("[Read]");
    appendColumn(ColRead);

    output << QString()
           << QStringLiteral("[Write]");
    appendColumn(ColWrite);

    if (mixed) {
        output << QString()
               << QStringLiteral("[Mix] Read %1%/Write %2%")
                  .arg(settings.getRandomReadPercentage())
                  .arg(100 - settings.getRandomReadPercentage());
        appendColumn(ColMix);
    }

    static const QString profiles[] = { QStringLiteral("Default"), QStringLiteral("Peak Performance"),
                                        QStringLiteral("Real World Performance"), QStringLiteral("Demo") };

    output << QString()
           << QStringLiteral("Profile: %1%2")
              .arg(profiles[(int)profile]).arg(mixed ? QStringLiteral(" [+Mix]") : QString())
           << QStringLiteral("   Test: %1")
              .arg(QStringLiteral("%1 %2 (x%3) [Measure: %4 %5 / Interval: %6 %7]")
              .arg(settings.getFileSize() >= 1024 ? settings.getFileSize() / 1024 : settings.getFileSize())
              .arg(settings.getFileSize() >= 1024 ? QStringLiteral("GiB") : QStringLiteral("MiB"))
              .arg(settings.getLoopsCount())
              .arg(settings.getMeasuringTime() >= 60 ? settings.getMeasuringTime() / 60 : settings.getMeasuringTime())
              .arg(settings.getMeasuringTime() >= 60 ? QStringLiteral("min") : QStringLiteral("sec"))
              .arg(settings.getIntervalTime() >= 60 ? settings.getIntervalTime() / 60 : settings.getIntervalTime())
              .arg(settings.getIntervalTime() >= 60 ? QStringLiteral("min") : QStringLiteral("sec")))
           << QStringLiteral("   Date: %1 %2")
              .arg(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")))
              .arg(QTime::currentTime().toString(QStringLiteral("hh:mm:ss")))
           << QStringLiteral("     OS: %1 %2 [%3 %4]").arg(QSysInfo::productType()).arg(QSysInfo::productVersion())
              .arg(QSysInfo::kernelType()).arg(QSysInfo::kernelVersion())
           << QStringLiteral(" Target: %1").arg(targetInfo());

    return output.join(QLatin1Char('\n'));
}

bool Tui::saveReport(const QString &fileName) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream fileStream(&file);
    fileStream << buildReport() << "\n";
    file.close();

    return true;
}

QString Tui::reportTestLine(const Global::BenchmarkParams &params, const Benchmark::PerformanceResult &result) const
{
    return QStringLiteral("%1 %2 %3 (Q=%4, T=%5): %6 MB/s [ %7 IOPS] < %8 us>")
           .arg(params.Pattern == Global::BenchmarkIOPattern::SEQ ? QStringLiteral("Sequential") : QStringLiteral("Random"))
           .arg(QString::number(params.BlockSize >= 1024 ? params.BlockSize / 1024 : params.BlockSize).rightJustified(3, ' '))
           .arg(params.BlockSize >= 1024 ? QStringLiteral("MiB") : QStringLiteral("KiB"))
           .arg(QString::number(params.Queues).rightJustified(3, ' '))
           .arg(QString::number(params.Threads).rightJustified(2, ' '))
           .arg(QString::number(result.Bandwidth, 'f', 3).rightJustified(9, ' '))
           .arg(QString::number(result.IOPS, 'f', 1).rightJustified(8, ' '))
           .arg(QString::number(result.Latency, 'f', 2).rightJustified(8, ' '))
           .rightJustified(Global::getOutputColumnsCount(), ' ');
}

QString Tui::targetInfo() const
{
    const QString dir = m_benchmark->getBenchmarkFile();
    const QStorageInfo volume(dir);

    QString info = dir;
    if (volume.bytesTotal() > 0) {
        const qlonglong occupied = volume.bytesTotal() - volume.bytesFree();
        info += QStringLiteral(" %1% (%2)").arg(occupied * 100 / volume.bytesTotal())
                                           .arg(formatSize(occupied, volume.bytesTotal()));
    }

    const QString modelName = DiskDriveInfo::Instance().getModelName(volume.device());
    if (!modelName.isEmpty())
        info += QStringLiteral(" [%1]").arg(modelName);

    return info;
}

QString Tui::formatSize(quint64 occupied, quint64 total) const
{
    // Recreated on each call, so that a language change retranslates the texts
    const QStringList units = { tr("Bytes"), tr("KiB"), tr("MiB"),
                                tr("GiB"), tr("TiB"), tr("PiB") };
    int i;
    double outputOccupied = occupied;
    double outputTotal = total;
    for (i = 0; i < units.size() - 1; i++) {
        if (outputTotal < 1024) break;
        outputOccupied = outputOccupied / 1024;
        outputTotal = outputTotal / 1024;
    }
    const QLocale locale;
    return QStringLiteral("%1/%2 %3").arg(locale.toString(outputOccupied, 'f', 2),
                                          locale.toString(outputTotal, 'f', 2), units.at(i));
}

QString Tui::fieldLabel(Field field) const
{
    switch (field) {
    case FieldStart:    return QString();
    case FieldProfile:  return tr("Profile");
    case FieldSize:     return tr("Size");
    case FieldLoops:    return tr("Loops");
    case FieldStorage:  return tr("Storage");
    case FieldUnit:     return tr("Unit");
    case FieldMixRatio: return tr("Mix ratio");
    case FieldLanguage: return tr("Language");
    }
    return QString();
}

QString Tui::fieldValueText(Field field) const
{
    const AppSettings settings;

    switch (field) {
    case FieldStart:
        return QString();

    case FieldProfile: {
        // Recreated on each call, so that a language change retranslates the texts
        const QString profiles[] = { tr("Default"), tr("Peak Performance"),
                                     tr("Real World Performance"), tr("Demo") };
        QString text = profiles[(int)settings.getPerformanceProfile()];
        if (isMixedActive())
            text += tr(" [+Mix]");
        return text;
    }

    case FieldSize: {
        const int fileSize = settings.getFileSize();
        return QStringLiteral("%1 %2")
               .arg(fileSize >= 1024 ? fileSize / 1024 : fileSize)
               .arg(fileSize >= 1024 ? tr("GiB") : tr("MiB"));
    }

    case FieldLoops:
        return QString::number(settings.getLoopsCount());

    case FieldStorage: {
        if (m_storageIndex < 0)
            return tr("Not available");
        const Global::Storage &storage = m_storages.at(m_storageIndex).storage;
        return QStringLiteral("%1 %2% (%3)")
               .arg(storage.path)
               .arg(storage.bytesTotal > 0 ? storage.bytesOccupied * 100 / storage.bytesTotal : 0)
               .arg(formatSize(storage.bytesOccupied, storage.bytesTotal));
    }

    case FieldUnit:
        return unitText();

    case FieldMixRatio:
        return QStringLiteral("R%1%/W%2%")
               .arg(settings.getRandomReadPercentage())
               .arg(100 - settings.getRandomReadPercentage());

    case FieldLanguage: {
        const QString languageName = QLocale().nativeLanguageName();
        if (languageName.isEmpty())
            return QStringLiteral("English");
        return languageName.at(0).toUpper() + languageName.mid(1);
    }
    }

    return QString();
}

QString Tui::unitText() const
{
    const QString units[] = { tr("MB/s"), tr("GB/s"), tr("IOPS"), tr("us") };
    return units[m_unitIndex];
}

float Tui::unitValue(const Benchmark::PerformanceResult &result) const
{
    switch ((Global::ComparisonUnit)m_unitIndex) {
    case Global::ComparisonUnit::MBPerSec: return result.Bandwidth;
    case Global::ComparisonUnit::GBPerSec: return result.Bandwidth / 1000;
    case Global::ComparisonUnit::IOPS:     return result.IOPS;
    case Global::ComparisonUnit::Latency:  return result.Latency;
    }
    return 0;
}

QString Tui::barText(const Benchmark::PerformanceResult &result) const
{
    float percent;

    // The same logarithmic scale as the GUI progress bars
    if ((Global::ComparisonUnit)m_unitIndex == Global::ComparisonUnit::Latency) {
        const float latency = result.Latency;
        percent = latency <= 0.0000000001f ? 0 : 100 - 16.666666f * std::log10(latency);
    }
    else {
        const float score = result.Bandwidth;
        percent = score <= 0.1f ? 0 : 16.666666f * std::log10(score * 10);
    }

    percent = qBound(0.0f, percent, 100.0f);

    const int barWidth = 10;
    const int filled = qRound(percent * barWidth / 100.0f);

    return QString(filled, QChar(0x2588)) + QString(barWidth - filled, QChar(0x2591)); // █░
}

QString Tui::shortRowLabel(const Global::BenchmarkParams &params) const
{
    return QStringLiteral("%1%2%3 Q%4T%5")
           .arg(params.Pattern == Global::BenchmarkIOPattern::SEQ ? QStringLiteral("SEQ") : QStringLiteral("RND"))
           .arg(params.BlockSize >= 1024 ? params.BlockSize / 1024 : params.BlockSize)
           .arg(params.BlockSize >= 1024 ? QStringLiteral("M") : QStringLiteral("K"))
           .arg(params.Queues)
           .arg(params.Threads);
}

void Tui::buildMainLines(QStringList &lines) const
{
    const AppSettings settings;
    const Global::PerformanceProfile profile = settings.getPerformanceProfile();
    const bool mixed = isMixedActive();
    const bool running = m_benchmark->isRunning();
    const QVector<Field> fields = visibleFields();
    const QLocale locale;

    // Title
    lines << QStringLiteral(" KDiskMark %1 | %2 | https://github.com/JonMagon/KDiskMark")
             .arg(QCoreApplication::applicationVersion(), m_benchmark->getFIOVersion());

    const QString divider(qBound(20, m_termWidth, 250), QChar(0x2500)); // ─
    lines << divider;

    // Settings fields
    int labelWidth = 0;
    for (Field field : fields)
        labelWidth = qMax(labelWidth, displayWidth(fieldLabel(field)));

    for (int i = 0; i < fields.size(); i++) {
        const Field field = fields.at(i);

        QString text;
        if (field == FieldStart)
            text = QStringLiteral("[ %1 ]").arg(running ? tr("Stop") : tr("Start All"));
        else
            text = QStringLiteral("%1  < %2 >").arg(padDisplay(fieldLabel(field), labelWidth),
                                                    fieldValueText(field));

        const bool focused = (m_focus == i) && !running;
        lines << QStringLiteral("  %1%2%3").arg(focused ? QStringLiteral("\x1b[7m") : QString(),
                                                text,
                                                focused ? QStringLiteral("\x1b[0m") : QString());

        if (field == FieldStorage && m_storageIndex >= 0) {
            const StorageEntry &entry = m_storages.at(m_storageIndex);
            QString deviceLine = entry.modelName;
            if (entry.encrypted) {
                deviceLine += (deviceLine.isEmpty() ? QString() : QStringLiteral(" | "))
                            + tr("The device is encrypted. Performance may drop.");
            }
            if (!deviceLine.isEmpty())
                lines << QStringLiteral("  %1\x1b[2m%2\x1b[0m").arg(QString(labelWidth + 2, QLatin1Char(' ')), deviceLine);
        }
    }

    lines << divider;

    // Results
    const int rowLabelWidth = 15;
    const int barWidth = 10;
    const int valueWidth = 9;
    const int columnWidth = 2 + barWidth + 1 + valueWidth;

    const QString unit = unitText();
    QString header = QString(2 + rowLabelWidth, QLatin1Char(' '));
    header += padDisplayLeft(QStringLiteral("%1 [%2]").arg(tr("Read"), unit), columnWidth);
    header += padDisplayLeft(QStringLiteral("%1 [%2]").arg(tr("Write"), unit), columnWidth);
    if (mixed)
        header += padDisplayLeft(QStringLiteral("%1 [%2]").arg(tr("Mix"), unit), columnWidth);
    lines << header;

    auto columnText = [&](const Benchmark::PerformanceResult &result) {
        const float value = unitValue(result);
        QString valueText;
        if (value >= 1000000.0f)
            valueText = locale.toString((int)value);
        else
            valueText = locale.toString(value, 'f', (Global::ComparisonUnit)m_unitIndex == Global::ComparisonUnit::GBPerSec ? 3 : 2);
        return QStringLiteral("  %1 %2").arg(barText(result), valueText.rightJustified(valueWidth));
    };

    for (int i = 0; i < m_rows.size(); i++) {
        const Row &row = m_rows.at(i);
        const Global::BenchmarkParams params = settings.getBenchmarkParams(row.test, profile);

        QString label = shortRowLabel(params).leftJustified(rowLabelWidth);
        const bool focused = (m_focus == fields.size() + i) && !running;
        if (focused)
            label = QStringLiteral("\x1b[7m") + label + QStringLiteral("\x1b[0m");

        QString line = QStringLiteral("  ") + label;
        line += columnText(row.results[ColRead]);
        line += columnText(row.results[ColWrite]);
        if (mixed)
            line += columnText(row.results[ColMix]);

        lines << line;
    }

    lines << divider;

    // Status area
    lines << QStringLiteral("  %1 %2").arg(tr("Status:"), m_status);
    if (!m_lastError.isEmpty())
        lines << QStringLiteral("  \x1b[31m%1 %2\x1b[0m").arg(tr("Error:"), m_lastError);
    if (!m_infoMessage.isEmpty())
        lines << QStringLiteral("  %1").arg(m_infoMessage);

    lines << QString();

    if (running) {
        lines << QStringLiteral("  \x1b[2m%1\x1b[0m").arg(tr("Enter Stop   Q Stop and quit"));
    }
    else {
        const bool onStorage = m_focus < fields.size() && fields.at(m_focus) == FieldStorage;
        lines << QStringLiteral("  \x1b[2m%1\x1b[0m")
                 .arg(onStorage ? tr("Up/Down Select   Left/Right Change   Enter Add directory   E Settings   Q Quit")
                                : tr("Up/Down Select   Left/Right Change   Enter Run   E Settings   S Save   R Rescan   Q Quit"));
    }
}

void Tui::buildSettingsLines(QStringList &lines) const
{
    const AppSettings settings;
    const Global::PerformanceProfile profile = settings.getPerformanceProfile();

    // Recreated on each call, so that a language change retranslates the texts
    const QString profiles[] = { tr("Default"), tr("Peak Performance"),
                                 tr("Real World Performance"), tr("Demo") };

    lines << QStringLiteral(" %1 - %2").arg(tr("Settings"), profiles[(int)profile]);

    const QString divider(qBound(20, m_termWidth, 250), QChar(0x2500)); // ─
    lines << divider;

    int item = 0;

    auto wrapFocus = [this, &item](const QString &text) {
        const QString result = m_settingsFocus == item ? QStringLiteral("\x1b[7m") + text + QStringLiteral("\x1b[0m")
                                                       : text;
        return result;
    };

    // Benchmark parameters of the current profile
    const QVector<Global::BenchmarkTest> tests = editableTests(profile);

    if (tests.isEmpty()) {
        lines << QStringLiteral("  %1").arg(tr("This profile uses fixed benchmark parameters."));
    }
    else {
        const QString columnTitles[4] = { tr("Pattern"), tr("Block size"), tr("Queues"), tr("Threads") };

        QVector<QString> rowLabels;
        QVector<QStringList> rowCells;
        for (int t = 0; t < tests.size(); t++) {
            const Global::BenchmarkParams params = settings.getBenchmarkParams(tests.at(t), profile);

            rowLabels << tr("Test %1").arg(t + 1);
            rowCells << QStringList {
                QStringLiteral("< %1 >").arg(params.Pattern == Global::BenchmarkIOPattern::SEQ ? QStringLiteral("SEQ")
                                                                                               : QStringLiteral("RND")),
                QStringLiteral("< %1 %2 >").arg(params.BlockSize >= 1024 ? params.BlockSize / 1024 : params.BlockSize)
                                           .arg(params.BlockSize >= 1024 ? tr("MiB") : tr("KiB")),
                QStringLiteral("< %1 >").arg(params.Queues),
                QStringLiteral("< %1 >").arg(params.Threads)
            };
        }

        // The columns are sized by their displayed content, so that translated
        // titles and values always keep a gap between them
        int labelWidth = 0;
        for (const QString &rowLabel : std::as_const(rowLabels))
            labelWidth = qMax(labelWidth, displayWidth(rowLabel));
        labelWidth += 2;

        int columnWidths[4];
        for (int c = 0; c < 4; c++) {
            int width = displayWidth(columnTitles[c]);
            for (const QStringList &cells : std::as_const(rowCells))
                width = qMax(width, displayWidth(cells.at(c)));
            columnWidths[c] = width + 2;
        }

        QString header = QString(2 + labelWidth, QLatin1Char(' '));
        for (int c = 0; c < 4; c++)
            header += padDisplay(columnTitles[c], columnWidths[c]);
        lines << header;

        for (int t = 0; t < tests.size(); t++) {
            QString line = QStringLiteral("  %1").arg(padDisplay(rowLabels.at(t), labelWidth));

            for (int c = 0; c < 4; c++) {
                const QString &cellText = rowCells.at(t).at(c);
                // The escape codes of the focus highlight do not occupy cells,
                // so the padding is appended outside of the highlighted text
                line += wrapFocus(cellText)
                      + QString(qMax(0, columnWidths[c] - displayWidth(cellText)), QLatin1Char(' '));
                item++;
            }

            lines << line;
        }
    }

    lines << divider;

    int scalarLabelWidth = 0;
    for (const QString &label : { tr("Measuring time"), tr("Interval time"), tr("Mode"), tr("Test data"),
                                  tr("Continuous generation"), tr("Flush pagecache"), tr("Use O_DIRECT"),
                                  tr("CoW detection") })
        scalarLabelWidth = qMax(scalarLabelWidth, displayWidth(label));

    auto scalarLine = [&](const QString &label, const QString &value) {
        const QString text = QStringLiteral("%1  < %2 >").arg(padDisplay(label, scalarLabelWidth), value);
        lines << QStringLiteral("  %1").arg(wrapFocus(text));
        item++;
    };

    scalarLine(tr("Measuring time"), timeText(settings.getMeasuringTime()));
    scalarLine(tr("Interval time"), timeText(settings.getIntervalTime()));

    const QString modes[] = { tr("All"), tr("Read Only"), tr("Write Only") };
    scalarLine(tr("Mode"), modes[(int)settings.getBenchmarkMode()]);

    scalarLine(tr("Test data"), settings.getBenchmarkTestData() == Global::BenchmarkTestData::Random
                                ? tr("Random") : tr("All Zeros"));
    scalarLine(tr("Continuous generation"), settings.getContinuousGenerationState() ? tr("On") : tr("Off"));
    scalarLine(tr("Flush pagecache"), settings.getFlusingCacheState() ? tr("On") : tr("Off"));
    scalarLine(tr("Use O_DIRECT"), settings.getCacheBypassState() ? tr("On") : tr("Off"));
    scalarLine(tr("CoW detection"), settings.getCoWDetectionState() ? tr("On") : tr("Off"));

    lines << QString();

    for (const QString &buttonText : { tr("Apply the Standard preset"), tr("Apply the NVMe SSD preset"), tr("Back") }) {
        lines << QStringLiteral("  %1").arg(wrapFocus(QStringLiteral("[ %1 ]").arg(buttonText)));
        item++;
    }

    lines << divider;

    if (!m_infoMessage.isEmpty())
        lines << QStringLiteral("  %1").arg(m_infoMessage);

    lines << QStringLiteral("  \x1b[2m%1\x1b[0m").arg(tr("Up/Down Select   Left/Right Change   Enter Apply   Esc Back"));
}

void Tui::render()
{
    if (!m_screenActive)
        return;

    QStringList lines;

    if (m_screen == SettingsScreen)
        buildSettingsLines(lines);
    else
        buildMainLines(lines);

    QString frame = QStringLiteral("\x1b[H");
    for (const QString &line : std::as_const(lines))
        frame += line + QStringLiteral("\x1b[K\n");
    frame += QStringLiteral("\x1b[J");

    QTextStream out(stdout);
    out << frame;
    out.flush();
}
