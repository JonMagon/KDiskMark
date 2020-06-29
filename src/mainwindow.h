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
    QThread benchmarkThread_;
    int waitSecondsBeforeNewTask_ = 5;
    bool isBenchmarkRunning_ = false; 

public:
    MainWindow(const AppSettings &settings, Benchmark *benchmark, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_SEQ1M_Q8T1_clicked();

    void on_pushButton_SEQ1M_Q1T1_clicked();

    void on_pushButton_RND4K_Q32T16_clicked();

    void on_pushButton_RND4K_Q1T1_clicked();

    void on_pushButton_All_clicked();

    void showAbout();

public slots:
    void benchmarkStatusUpdated(const QString &name);
    void handleResults(QProgressBar *progressBar, const Benchmark::PerformanceResult &result);
    void timeIntervalSelected(QAction* act);
    void isBenchmarkRunning(bool *state);
    void allTestsAreFinished();

signals:
    void runBenchmark(QMap<Benchmark::Type, QProgressBar*>, int loops, int intervalTime);

private:
    Ui::MainWindow *ui;
    bool changeTaskState();
    void closeEvent(QCloseEvent *event);
};
#endif // MAINWINDOW_H
