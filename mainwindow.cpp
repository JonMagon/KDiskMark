#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QProcess>
#include <QToolTip>
#include <QMessageBox>
#include <QThread>
#include "math.h"

#include "benchmark.h"

static int waitSecondsBeforeNewTask = 5;
const QString toolTipRaw = "<h1>%1 MB/s<br/>%2 GB/s<br/>%3 IOPS<br/>%4 μs</h1>";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    statusBar()->setSizeGripEnabled(false);

    QActionGroup *timeIntervalGroup = new QActionGroup(this);

    ui->action0_sec->setProperty("interval", 0);
    ui->action1_sec->setProperty("interval", 1);
    ui->action3_sec->setProperty("interval", 3);
    ui->action5_sec->setProperty("interval", 5);
    ui->action10_sec->setProperty("interval", 10);
    ui->action30_sec->setProperty("interval", 30);
    ui->action1_min->setProperty("interval", 60);
    ui->action3_min->setProperty("interval", 180);
    ui->action5_min->setProperty("interval", 300);
    ui->action10_min->setProperty("interval", 600);

    ui->action0_sec->setActionGroup(timeIntervalGroup);
    ui->action1_sec->setActionGroup(timeIntervalGroup);
    ui->action3_sec->setActionGroup(timeIntervalGroup);
    ui->action5_sec->setActionGroup(timeIntervalGroup);
    ui->action10_sec->setActionGroup(timeIntervalGroup);
    ui->action30_sec->setActionGroup(timeIntervalGroup);
    ui->action1_min->setActionGroup(timeIntervalGroup);
    ui->action3_min->setActionGroup(timeIntervalGroup);
    ui->action5_min->setActionGroup(timeIntervalGroup);
    ui->action10_min->setActionGroup(timeIntervalGroup);

    connect(timeIntervalGroup, SIGNAL(triggered(QAction*)), this, SLOT(timeIntervalSelected(QAction*)));

    int size = 16;
    for (int i = 0; i < 10; i++) {
        size *= 2;
        ui->comboBox->addItem(QString("%1 %2").arg(QString::number(size), tr("MiB")));
    }

    QProgressBar* progressBars[] = {
        ui->readBar_SEQ1M_Q8T1, ui->writeBar_SEQ1M_Q8T1,
        ui->readBar_SEQ1M_Q1T1, ui->writeBar_SEQ1M_Q1T1,
        ui->readBar_RND4K_Q32T16, ui->writeBar_RND4K_Q32T16,
        ui->readBar_RND4K_Q1T1, ui->writeBar_RND4K_Q1T1
    };

    for (auto const& progressBar: progressBars) {
        progressBar->setFormat("0.00");
        progressBar->setToolTip(toolTipRaw.arg("0.000", "0.000", "0.000", "0.000"));
    }

    ui->pushButton_SEQ1M_Q8T1->setToolTip("<h1>Sequential 1 MiB/s<br/>Queues=8<br/>Threads=1</h1>");

    Benchmark *benchmark = new Benchmark;
    benchmark->moveToThread(&benchmarkThread);
    connect(&benchmarkThread, &QThread::finished, benchmark, &QObject::deleteLater);
    connect(this, &MainWindow::runBenchmark, benchmark, &Benchmark::runBenchmark);
    connect(this, &MainWindow::waitTask, benchmark, &Benchmark::waitTask);
    connect(benchmark, &Benchmark::benchmarkStatusUpdated, this, &MainWindow::benchmarkStatusUpdated);
    connect(benchmark, &Benchmark::resultReady, this, &MainWindow::handleResults);
    benchmarkThread.start();
}

MainWindow::~MainWindow()
{
    benchmarkThread.quit();
    benchmarkThread.wait();
    delete ui;
}

void MainWindow::timeIntervalSelected(QAction* act)
{
    waitSecondsBeforeNewTask = act->property("interval").toInt();
}

void MainWindow::benchmarkStatusUpdated(const QString &name)
{
    setWindowTitle(QString("KDiskMark - %1").arg(name));
}

void MainWindow::handleResults(QProgressBar *progressBar, const Benchmark::PerformanceResult &result)
{
    progressBar->setFormat(QString::number(result.Bandwidth, 'f', 2));
    progressBar->setValue(16.666666666666 * log10(result.Bandwidth * 10));
    progressBar->setToolTip(
                toolTipRaw.arg(
                    QString::number(result.Bandwidth, 'f', 3),
                    QString::number(result.Bandwidth / 1000, 'f', 3),
                    QString::number(result.IOPS, 'f', 3),
                    QString::number(result.Latency, 'f', 3)
                    )
                );

    ui->pushButton_All->setEnabled(true);
    setWindowTitle("KDiskMark");
}

void MainWindow::on_pushButton_SEQ1M_Q8T1_clicked()
{
    runBenchmark(ui->readBar_SEQ1M_Q8T1, Benchmark::SEQ1M_Q8T1_Read, ui->loopsCount->value());
    waitTask(waitSecondsBeforeNewTask);
    runBenchmark(ui->writeBar_SEQ1M_Q8T1, Benchmark::SEQ1M_Q8T1_Write, ui->loopsCount->value());
}

void MainWindow::on_pushButton_SEQ1M_Q1T1_clicked()
{
    runBenchmark(ui->readBar_SEQ1M_Q1T1, Benchmark::SEQ1M_Q1T1_Read, ui->loopsCount->value());
    waitTask(waitSecondsBeforeNewTask);
    runBenchmark(ui->writeBar_SEQ1M_Q1T1, Benchmark::SEQ1M_Q1T1_Write, ui->loopsCount->value());
}

void MainWindow::on_pushButton_RND4K_Q32T16_clicked()
{
    runBenchmark(ui->readBar_RND4K_Q32T16, Benchmark::RND4K_Q32T16_Read, ui->loopsCount->value());
    waitTask(waitSecondsBeforeNewTask);
    runBenchmark(ui->writeBar_RND4K_Q32T16, Benchmark::RND4K_Q32T16_Write, ui->loopsCount->value());
}

void MainWindow::on_pushButton_RND4K_Q1T1_clicked()
{
    runBenchmark(ui->readBar_RND4K_Q1T1, Benchmark::RND4K_Q1T1_Read, ui->loopsCount->value());
    waitTask(waitSecondsBeforeNewTask);
    runBenchmark(ui->writeBar_RND4K_Q1T1, Benchmark::RND4K_Q1T1_Write, ui->loopsCount->value());
}

void MainWindow::on_pushButton_All_clicked()
{
    ui->pushButton_All->setEnabled(false);
}
