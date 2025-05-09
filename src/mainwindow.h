#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "benchmark.h"
#include "appsettings.h"

class QComboBox;
class QProgressBar;
class QStorageInfo;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_Test_1_clicked();

    void on_pushButton_Test_2_clicked();

    void on_pushButton_Test_3_clicked();

    void on_pushButton_Test_4_clicked();

    void on_pushButton_All_clicked();

    void on_actionAbout_triggered();

    void on_actionQueues_Threads_triggered();

    void on_comboBox_Storages_currentIndexChanged(int index);

    void on_loopsCount_valueChanged(int arg1);

    void on_comboBox_ComparisonUnit_currentIndexChanged(int index);

    void on_actionCopy_triggered();

    void on_actionSave_triggered();

    void on_comboBox_MixRatio_currentIndexChanged(int index);

    void on_refreshStoragesButton_clicked();

    void on_comboBox_fileSize_currentIndexChanged(int index);

    void on_actionFlush_Pagecache_triggered(bool checked);

    void on_actionUse_O_DIRECT_triggered(bool checked);

private:
    Ui::MainWindow *ui;
    Benchmark *m_benchmark;
    QVector<QProgressBar*> m_progressBars;
    QString m_windowTitle;

    void updateFileSizeList();
    void updateStoragesList();
    void addItemToStoragesList(const Global::Storage &storage);
    void defineBenchmark(std::function<void()> bodyFunc);
    void closeEvent(QCloseEvent *event);
    QString formatSize(quint64 available, quint64 total);
    QString getTextBenchmarkResult();
    void updateBenchmarkButtonsContent();
    void updatePresetsSelection();
    void refreshProgressBars();
    void updateProgressBar(QProgressBar *progressBar);
    void updateLabels();
    bool runCombinedRandomTest();
    QString combineOutputTestResult(const QProgressBar *progressBar, const Global::BenchmarkParams &params);
    void resizeComboBoxItemsPopup(QComboBox *combobox);
    void updateProgressBarsStyle();
    void handleDirectoryChanged(const QString &newDir);

public slots:
    void benchmarkStatusUpdate(const QString &name);
    void benchmarkFailed(const QString &error);
    void handleResults(QProgressBar *progressBar, const Benchmark::PerformanceResult &result);
    void localeSelected(QAction* act);
    void profileSelected(QAction* act);
    void modeSelected(QAction* act);
    void testDataSelected(QAction* act);
    void presetSelected(QAction* act);
    void themeSelected(QAction* act);
    void benchmarkStateChanged(bool state);
    void handleCowCheck();

protected slots:
    virtual void changeEvent(QEvent * event);
};
#endif // MAINWINDOW_H
