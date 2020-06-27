#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QProgressBar>
#include <QThreadPool>

#include "benchmark.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QThread benchmarkThread;
    int waitSecondsBeforeNewTask = 5;
    const QString toolTipRaw = tr("<h1>%1 MiB/s<br/>%2 GiB/s<br/>%3 IOPS<br/>%4 μs</h1>");

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_SEQ1M_Q8T1_clicked();

    void on_pushButton_SEQ1M_Q1T1_clicked();

    void on_pushButton_RND4K_Q32T16_clicked();

    void on_pushButton_RND4K_Q1T1_clicked();

    void on_pushButton_All_clicked();

public slots:
    void benchmarkStatusUpdated(const QString &name);
    void handleResults(QProgressBar *progressBar, const Benchmark::PerformanceResult &result);
    void timeIntervalSelected(QAction* act);

signals:
    void runBenchmark(QProgressBar *progressBar, Benchmark::Type type, int loops);
    void waitTask(int sec);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
