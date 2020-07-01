#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QProgressBar>
#include <QThreadPool>

#include "benchmark.h"
#include "appsettings.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    const QString toolTipRaw = tr("<h1>%1 MiB/s<br/>%2 GiB/s<br/>%3 IOPS<br/>%4 μs</h1>");
    Benchmark *m_benchmark;
    AppSettings *m_settings;
    QThread m_benchmarkThread;
    bool m_isBenchmarkThreadRunning = false;

public:
    MainWindow(AppSettings *settings, Benchmark *benchmark, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_SEQ1M_Q8T1_clicked();

    void on_pushButton_SEQ1M_Q1T1_clicked();

    void on_pushButton_RND4K_Q32T16_clicked();

    void on_pushButton_RND4K_Q1T1_clicked();

    void on_pushButton_All_clicked();

    void showAbout();

    void on_comboBox_FSPoints_currentIndexChanged(int index);

    void on_loopsCount_valueChanged(int arg1);

public slots:
    void benchmarkStatusUpdated(const QString &name);
    void benchmarkFailed(const QString &error);
    void handleResults(QProgressBar *progressBar, const Benchmark::PerformanceResult &result);
    void timeIntervalSelected(QAction* act);
    void benchmarkStateChanged(bool state);

signals:
    void runBenchmark(QMap<Benchmark::Type, QProgressBar*>);

private:
    Ui::MainWindow *ui;
    void runOrStopBenchmarkThread();
    void closeEvent(QCloseEvent *event);
    QString formatSize(quint64 available, quint64 total);
    bool disableDirItemIfIsNotWritable(int index);
};
#endif // MAINWINDOW_H
