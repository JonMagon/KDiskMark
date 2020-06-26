#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QProcess>
#include <QToolTip>
#include "math.h"

#include "benchmark.h"

const QString toolTipRaw = "<h1>%1 MB/s<br/>%2 GB/s<br/>%3 IOPS<br/>%4 μs</h1>";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    int size = 16;
    for (int i = 0; i < 10; i++) {
        size *= 2;
        ui->comboBox->addItem(QString("%1 %2").arg(QString::number(size), "MiB"));
    }

    QProgressBar* progressBars[] = {
        ui->readBar_SEQ1M_Q8T1, ui->writeBar_SEQ1M_Q8T1,
        ui->readBar_SEQ1M_Q1T1, ui->writeBar_SEQ1M_Q1T1,
        ui->readBar_RND4K_Q32T16, ui->writeBar_RND4K_Q32T16,
        ui->readBar_RND4K_Q1T1, ui->writeBar_RND4K_Q1T1
    };
    for (auto const& progressBar: progressBars) {
        progressBar->setFormat("0.00");
        progressBar->setToolTip(toolTipRaw.arg("0.000").arg("0.000").arg("0.000").arg("0.000"));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SetBenchmarkResult(QProgressBar* progressBar, Benchmark::PerformanceResult& result)
{
    progressBar->setFormat(QString::number(result.Bandwidth, 'f', 2));
    progressBar->setValue(16.666666666666 * log10(result.Bandwidth * 10));
    progressBar->setToolTip(
                toolTipRaw
                .arg(QString::number(result.Bandwidth, 'f', 3))
                .arg(QString::number(result.Bandwidth / 1000, 'f', 3))
                .arg(QString::number(result.IOPS, 'f', 3))
                .arg(QString::number(result.Latency, 'f', 3))
                );

    qApp->processEvents();
}

void MainWindow::on_pushButton_SEQ1M_Q8T1_clicked()
{
    Benchmark::PerformanceResult result;

    Benchmark::Instance().SEQ1M_Q8T1_Read(result, ui->loopsCount->value());
    SetBenchmarkResult(ui->readBar_SEQ1M_Q8T1, result);

    Benchmark::Instance().SEQ1M_Q8T1_Write(result, ui->loopsCount->value());
    SetBenchmarkResult(ui->writeBar_SEQ1M_Q8T1, result);
}

void MainWindow::on_pushButton_SEQ1M_Q1T1_clicked()
{
    Benchmark::PerformanceResult result;

    Benchmark::Instance().SEQ1M_Q1T1_Read(result, ui->loopsCount->value());
    SetBenchmarkResult(ui->readBar_SEQ1M_Q1T1, result);

    Benchmark::Instance().SEQ1M_Q1T1_Write(result, ui->loopsCount->value());
    SetBenchmarkResult(ui->writeBar_SEQ1M_Q1T1, result);
}

void MainWindow::on_pushButton_RND4K_Q32T16_clicked()
{
    Benchmark::PerformanceResult result;

    Benchmark::Instance().RND4K_Q32T16_Read(result, ui->loopsCount->value());
    SetBenchmarkResult(ui->readBar_RND4K_Q32T16, result);

    Benchmark::Instance().RND4K_Q32T16_Write(result, ui->loopsCount->value());
    SetBenchmarkResult(ui->writeBar_RND4K_Q32T16, result);
}

void MainWindow::on_pushButton_RND4K_Q1T1_clicked()
{
    Benchmark::PerformanceResult result;

    Benchmark::Instance().RND4K_Q1T1_Read(result, ui->loopsCount->value());
    SetBenchmarkResult(ui->readBar_RND4K_Q1T1, result);

    Benchmark::Instance().RND4K_Q1T1_Write(result, ui->loopsCount->value());
    SetBenchmarkResult(ui->writeBar_RND4K_Q1T1, result);
}
