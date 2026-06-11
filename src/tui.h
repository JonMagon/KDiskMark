#ifndef TUI_H
#define TUI_H

#include <termios.h>

#include <QHash>
#include <QObject>
#include <QPair>
#include <QVector>

#include "benchmark.h"
#include "global.h"

class QCoreApplication;
class QProcess;
class QSocketNotifier;

class Tui : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Tui)

public:
    explicit Tui(QObject *parent = nullptr);
    ~Tui() override;

    int run(QCoreApplication &app);

public slots:
    void benchmarkStatusUpdate(const QString &name);
    void handleResults(QObject *target, const Benchmark::PerformanceResult &result);
    void benchmarkFailed(const QString &error);
    void handleCowCheck();
    void handleDirectoryChanged(const QString &newDir);
    void handleUnixSignal();
    void handleKeyInput();

private:
    enum Column { ColRead = 0, ColWrite, ColMix, ColCount };

    struct Row
    {
        Global::BenchmarkTest test;
        Benchmark::PerformanceResult results[ColCount];
    };

    enum Screen { MainScreen, SettingsScreen };

    enum Field {
        FieldStart = 0,
        FieldProfile,
        FieldSize,
        FieldLoops,
        FieldStorage,
        FieldUnit,
        FieldMixRatio,
        FieldLanguage
    };

    struct SettingsItem
    {
        enum Kind {
            TestPattern, TestBlockSize, TestQueues, TestThreads,
            MeasureTime, IntervalTime, Mode, TestData, Continuous,
            FlushCache, CacheBypass, CowDetection,
            PresetStandard, PresetNvme, Back
        };

        Kind kind;
        Global::BenchmarkTest test = Global::BenchmarkTest::Test_1; // for the Test* kinds
    };

    struct StorageEntry
    {
        Global::Storage storage;
        QString modelName;
        bool encrypted = false;
    };

    // Terminal handling
    void enterScreen();
    void leaveScreen();
    void updateTerminalSize();
    QString promptLine(const QString &prompt);
    void processKey(int key);

    // Main screen
    void buildMainLines(QStringList &lines) const;
    QVector<Field> visibleFields() const;
    int focusItemsCount() const;
    void moveFocus(int delta);
    void changeFieldValue(Field field, int direction);
    void activateFocused(bool enterKey);
    void promptForDirectory();
    QString fieldLabel(Field field) const;
    QString fieldValueText(Field field) const;

    // Settings screen
    void openSettingsScreen();
    void buildSettingsLines(QStringList &lines) const;
    void buildSettingsItems();
    void changeSettingsValue(const SettingsItem &item, int direction);
    void activateSettingsItem();
    void applyPresetToSettings(Global::BenchmarkPreset preset);
    QString timeText(int seconds) const;

    // Benchmarking
    void startBenchmark(int onlyRow);
    void stopOrQuit(bool quit);
    void rescanStorages(const QString &preferredPath = QString());
    void selectStorage(int index);
    void addCustomStorage(const QString &path, bool select);
    void rebuildRows();
    void buildTestPlan(int onlyRow = -1);
    bool storeResult(QObject *target, const Benchmark::PerformanceResult &result);
    void startPolkitTtyAgent();
    bool authorize(QString *errorMessage);
    void setupUnixSignalHandlers();

    // Report
    void saveReportInteractive();
    QString buildReport() const;
    bool saveReport(const QString &fileName) const;
    QString reportTestLine(const Global::BenchmarkParams &params, const Benchmark::PerformanceResult &result) const;
    QString targetInfo() const;
    QString formatSize(quint64 occupied, quint64 total) const;

    // Display helpers
    void render();
    QString shortRowLabel(const Global::BenchmarkParams &params) const;
    QString unitText() const;
    float unitValue(const Benchmark::PerformanceResult &result) const;
    QString barText(const Benchmark::PerformanceResult &result) const;
    static bool isMixedActive();
    static QVector<Global::BenchmarkTest> profileTests(Global::PerformanceProfile profile);
    static QVector<Global::BenchmarkTest> editableTests(Global::PerformanceProfile profile);

    Benchmark *m_benchmark = nullptr;
    QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QObject*>>> m_testPlan;
    QVector<Row> m_rows;
    QHash<QObject*, QPair<int, int>> m_cells; // target -> (row, column)
    QVector<StorageEntry> m_storages;
    QVector<SettingsItem> m_settingsItems;
    QProcess *m_polkitAgent = nullptr;
    QSocketNotifier *m_signalNotifier = nullptr;
    QSocketNotifier *m_keyNotifier = nullptr;
    QString m_status;
    QString m_lastError;
    QString m_infoMessage;
    Screen m_screen = MainScreen;
    int m_storageIndex = -1;
    int m_focus = 0;
    int m_settingsFocus = 0;
    int m_termWidth = 80;
    int m_termHeight = 24;
    int m_unitIndex = 0; // Global::ComparisonUnit
    struct termios m_originalTermios = {};
    bool m_screenActive = false;
    bool m_quitRequested = false;
    bool m_stopIssued = false;
    bool m_stopRequested = false;
    int m_exitCode = 0;

    static const QVector<int> s_fileSizes;
    static const QVector<int> s_blockSizes;
    static const QVector<int> s_queues;
    static const QVector<int> s_measureTimes;
    static const QVector<int> s_intervalTimes;
};

#endif // TUI_H
