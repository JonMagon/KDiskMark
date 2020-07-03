#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QProgressBar>
#include <QThreadPool>

#include "benchmark.h"
#include "appsettings.h"

class QStorageInfo;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    Benchmark *m_benchmark;
    AppSettings *m_settings;
    QThread m_benchmarkThread;
    bool m_isBenchmarkThreadRunning = false;
    QVector<QProgressBar*> m_progressBars;

public:
    MainWindow(AppSettings *settings, Benchmark *benchmark, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_SEQ_1_clicked();

    void on_pushButton_SEQ_2_clicked();

    void on_pushButton_RND_1_clicked();

    void on_pushButton_RND_2_clicked();

    void on_pushButton_All_clicked();

    void showAbout();

    void showSettings();

    void on_comboBox_Dirs_currentIndexChanged(int index);

    void on_loopsCount_valueChanged(int arg1);

    void on_comboBox_ComparisonField_currentIndexChanged(int index);

    void copyBenchmarkResult();

public slots:
    void benchmarkStatusUpdate(const QString &name);
    void benchmarkFailed(const QString &error);
    void handleResults(QProgressBar *progressBar, const Benchmark::PerformanceResult &result);
    void timeIntervalSelected(QAction* act);
    void benchmarkStateChanged(bool state);

signals:
    void runBenchmark(QList<QPair<Benchmark::Type, QProgressBar*>>);

private:
    Ui::MainWindow *ui;
    void runOrStopBenchmarkThread();
    void closeEvent(QCloseEvent *event);
    QString formatSize(quint64 available, quint64 total);
    bool disableDirItemIfIsNotWritable(int index);
    void updateBenchmarkButtonsContent();
    void updateProgressBar(QProgressBar *progressBar);
    void combineOutputTestResult(QString &output, const QString &name, const QProgressBar *progressBar,
                                 const AppSettings::BenchmarkParams &params);
};
#endif // MAINWINDOW_H
