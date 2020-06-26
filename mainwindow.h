#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QProgressBar>
#include "benchmark.h"

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
    void on_pushButton_SEQ1M_Q8T1_clicked();

    void on_pushButton_SEQ1M_Q1T1_clicked();

    void on_pushButton_RND4K_Q32T16_clicked();

    void on_pushButton_RND4K_Q1T1_clicked();

private:
    Ui::MainWindow *ui;
    void SetBenchmarkResult(QProgressBar* progressBar, Benchmark::PerformanceResult& result);
};
#endif // MAINWINDOW_H
