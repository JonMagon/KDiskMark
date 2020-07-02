#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QProcess>
#include <QToolTip>
#include <QMessageBox>
#include <QThread>
#include <QStorageInfo>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QMetaEnum>

#include "math.h"
#include "about.h"
#include "settings.h"
#include "global.h"

MainWindow::MainWindow(AppSettings *settings, Benchmark *benchmark, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(Global::Instance().getIconSVGPath()));

    statusBar()->setSizeGripEnabled(false);

    ui->loopsCount->findChild<QLineEdit*>()->setReadOnly(true);

    m_settings = settings;

    // Default values
    ui->loopsCount->setValue(m_settings->getLoopsCount());

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

    for (int i = 16; i <= 512; i *= 2) {
        ui->comboBox_fileSize->addItem(QStringLiteral("%1 %2").arg(i).arg(tr("MiB")), i);
    }

    for (int i = 1; i <= 64; i *= 2) {
        ui->comboBox_fileSize->addItem(QStringLiteral("%1 %2").arg(i).arg(tr("GiB")), i * 1024);
    }

    ui->comboBox_fileSize->
            setCurrentIndex(ui->comboBox_fileSize->findData(m_settings->getFileSize()));

    m_progressBars << ui->readBar_SEQ_1 << ui->writeBar_SEQ_1 << ui->readBar_SEQ_2 << ui->writeBar_SEQ_2
                   << ui->readBar_RND_1 << ui->writeBar_RND_1 << ui->readBar_RND_2 << ui->writeBar_RND_2;

    QMetaEnum metaEnum = QMetaEnum::fromType<AppSettings::ComparisonField>();

    for (auto const& progressBar: m_progressBars) {
        progressBar->setProperty(metaEnum.valueToKey(AppSettings::MiBPerSec), 0);
        progressBar->setProperty(metaEnum.valueToKey(AppSettings::GiBPerSec), 0);
        progressBar->setProperty(metaEnum.valueToKey(AppSettings::IOPS), 0);
        progressBar->setProperty(metaEnum.valueToKey(AppSettings::Latency), 0);
        progressBar->setFormat("0.00");
        progressBar->setToolTip(Global::Instance().getToolTipTemplate().arg("0.000", "0.000", "0.000", "0.000"));
    }

    updateBenchmarkButtonsContent();

    bool isSomeDeviceMountAsHome = false;
    bool isThereWritableDir = false;

    // Add each device and its mount point if is writable
    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady() && !storage.isReadOnly()) {
            if (!(storage.device().indexOf("/dev/sd") == -1 && storage.device().indexOf("/dev/nvme") == -1)) {

                if (storage.rootPath() == QDir::homePath())
                    isSomeDeviceMountAsHome = true;

                quint64 total = storage.bytesTotal();
                quint64 available = storage.bytesAvailable();

                QString path = storage.rootPath();

                ui->comboBox_Dirs->addItem(
                            tr("%1 %2% (%3)").arg(path)
                            .arg(available * 100 / total)
                            .arg(formatSize(available, total)),
                            path
                            );

                if (!disableDirItemIfIsNotWritable(ui->comboBox_Dirs->count() - 1)
                        && isThereWritableDir == false) {
                    isThereWritableDir = true;
                }
            }
        }
    }

    // Add home dir
    if (!isSomeDeviceMountAsHome) {
        QStorageInfo storage = QStorageInfo::root();

        quint64 total = storage.bytesTotal();
        quint64 available = storage.bytesAvailable();

        QString path = QDir::homePath();

        ui->comboBox_Dirs->insertItem(0,
                    tr("%1 %2% (%3)").arg(path)
                    .arg(storage.bytesAvailable() * 100 / total)
                    .arg(formatSize(available, total)),
                    path
                    );

        if (!disableDirItemIfIsNotWritable(0)
                && isThereWritableDir == false) {
            isThereWritableDir = true;
        }
    }

    if (isThereWritableDir) {
        ui->comboBox_Dirs->setCurrentIndex(0);
    }
    else {
        ui->comboBox_Dirs->setCurrentIndex(-1);
        ui->pushButton_All->setEnabled(false);
        ui->pushButton_SEQ_1->setEnabled(false);
        ui->pushButton_SEQ_2->setEnabled(false);
        ui->pushButton_RND_1->setEnabled(false);
        ui->pushButton_RND_2->setEnabled(false);
    }

    // Move Benchmark to another thread and set callbacks
    m_benchmark = benchmark;
    benchmark->moveToThread(&m_benchmarkThread);
    connect(this, &MainWindow::runBenchmark, m_benchmark, &Benchmark::runBenchmark);
    connect(m_benchmark, &Benchmark::runningStateChanged, this, &MainWindow::benchmarkStateChanged);
    connect(m_benchmark, &Benchmark::benchmarkStatusUpdate, this, &MainWindow::benchmarkStatusUpdate);
    connect(m_benchmark, &Benchmark::resultReady, this, &MainWindow::handleResults);
    connect(m_benchmark, &Benchmark::failed, this, &MainWindow::benchmarkFailed);
    connect(m_benchmark, &Benchmark::finished, &m_benchmarkThread, &QThread::terminate);

    // About button
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);

    // Settings
    connect(ui->actionQueues_Threads, &QAction::triggered, this, &MainWindow::showSettings);
}

MainWindow::~MainWindow()
{
    m_benchmarkThread.quit();
    m_benchmarkThread.wait();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *)
{
    m_benchmark->setRunning(false);
}

void MainWindow::updateBenchmarkButtonsContent()
{
    AppSettings::BenchmarkParams params;

    params = m_settings->SEQ_1;
    ui->pushButton_SEQ_1->setText(QStringLiteral("SEQ%1M\nQ%2T%3").arg(params.BlockSize / 1024)
                                  .arg(params.Queues).arg(params.Threads));
    ui->pushButton_SEQ_1->setToolTip(tr("<h2>Sequential %1 MiB<br/>Queues=%2<br/>Threads=%3</h2>")
                                      .arg(params.BlockSize / 1024).arg(params.Queues).arg(params.Threads));

    params = m_settings->SEQ_2;
    ui->pushButton_SEQ_2->setText(QStringLiteral("SEQ%1M\nQ%2T%3").arg(params.BlockSize / 1024)
                                  .arg(params.Queues).arg(params.Threads));
    ui->pushButton_SEQ_2->setToolTip(tr("<h2>Sequential %1 MiB<br/>Queues=%2<br/>Threads=%3</h2>")
                                     .arg(params.BlockSize / 1024).arg(params.Queues).arg(params.Threads));

    params = m_settings->RND_1;
    ui->pushButton_RND_1->setText(QStringLiteral("RND%1K\nQ%2T%3").arg(params.BlockSize)
                                  .arg(params.Queues).arg(params.Threads));
    ui->pushButton_RND_1->setToolTip(tr("<h2>Random %1 KiB<br/>Queues=%2<br/>Threads=%3</h2>")
                                     .arg(params.BlockSize).arg(params.Queues).arg(params.Threads));

    params = m_settings->RND_2;
    ui->pushButton_RND_2->setText(QStringLiteral("RND%1K\nQ%2T%3").arg(params.BlockSize)
                                  .arg(params.Queues).arg(params.Threads));
    ui->pushButton_RND_2->setToolTip(tr("<h2>Random %1 KiB<br/>Queues=%2<br/>Threads=%3</h2>")
                                     .arg(params.BlockSize).arg(params.Queues).arg(params.Threads));
}

bool MainWindow::disableDirItemIfIsNotWritable(int index)
{
    if (!QFileInfo(ui->comboBox_Dirs->itemData(index).toString()).isWritable()) {
        const QStandardItemModel* model =
                dynamic_cast<QStandardItemModel*>(ui->comboBox_Dirs->model());
        QStandardItem* item = model->item(index);
        item->setEnabled(false);

        return true;
    }
    else return false;
}

QString MainWindow::formatSize(quint64 available, quint64 total)
{
    QStringList units = { tr("Bytes"), tr("KiB"), tr("MiB"), tr("GiB"), tr("TiB"), tr("PiB") };
    int i;
    double outputAvailable = available;
    double outputTotal = total;
    for(i = 0; i < units.size() - 1; i++) {
        if (outputTotal < 1024) break;
        outputAvailable = outputAvailable / 1024;
        outputTotal = outputTotal / 1024;
    }
    return QString("%1/%2 %3").arg(outputAvailable, 0, 'f', 2)
            .arg(outputTotal, 0, 'f', 2).arg(units[i]);
}

void MainWindow::on_comboBox_ComparisonField_currentIndexChanged(int index)
{
    m_settings->comprasionField = AppSettings::ComparisonField(index);

    for (auto const& progressBar: m_progressBars) {
        updateProgressBar(progressBar);
    }
}

void MainWindow::timeIntervalSelected(QAction* act)
{
    m_settings->setIntervalTime(act->property("interval").toInt());
}

void MainWindow::on_loopsCount_valueChanged(int arg1)
{
    m_settings->setLoopsCount(arg1);
}

void MainWindow::on_comboBox_Dirs_currentIndexChanged(int index)
{
    m_settings->setDir(ui->comboBox_Dirs->itemData(index).toString());
}

void MainWindow::benchmarkStateChanged(bool state)
{
    if (state) {
        ui->menubar->setEnabled(false);
        ui->loopsCount->setEnabled(false);
        ui->comboBox_fileSize->setEnabled(false);
        ui->comboBox_Dirs->setEnabled(false);
        ui->comboBox_ComparisonField->setEnabled(false);
        ui->pushButton_All->setText(tr("Stop"));
        ui->pushButton_SEQ_1->setText(tr("Stop"));
        ui->pushButton_SEQ_2->setText(tr("Stop"));
        ui->pushButton_RND_1->setText(tr("Stop"));
        ui->pushButton_RND_2->setText(tr("Stop"));
    }
    else {
        setWindowTitle(qAppName());
        ui->menubar->setEnabled(true);
        ui->loopsCount->setEnabled(true);
        ui->comboBox_fileSize->setEnabled(true);
        ui->comboBox_Dirs->setEnabled(true);
        ui->comboBox_ComparisonField->setEnabled(true);
        ui->pushButton_All->setText(tr("All"));
        ui->pushButton_SEQ_1->setText("SEQ1M\nQ8T1");
        ui->pushButton_SEQ_2->setText("SEQ1M\nQ1T1");
        ui->pushButton_RND_1->setText("RND4K\nQ32T16");
        ui->pushButton_RND_2->setText("RND4K\nQ1T1");
        ui->pushButton_All->setEnabled(true);
        ui->pushButton_SEQ_1->setEnabled(true);
        ui->pushButton_SEQ_2->setEnabled(true);
        ui->pushButton_RND_1->setEnabled(true);
        ui->pushButton_RND_2->setEnabled(true);
    }

    m_isBenchmarkThreadRunning = state;
}

void MainWindow::showAbout()
{
    About about(m_benchmark->FIOVersion());
    about.setFixedSize(about.size());
    about.exec();
}

void MainWindow::showSettings()
{
    Settings settings(m_settings);
    settings.setFixedSize(settings.size());
    settings.exec();

    updateBenchmarkButtonsContent();
}

void MainWindow::runOrStopBenchmarkThread()
{
    if (m_isBenchmarkThreadRunning) {
        m_benchmark->setRunning(false);
        benchmarkStatusUpdate(tr("Stopping..."));
    }
    else {
        if (m_settings->getBenchmarkFile().isNull()) {
            QMessageBox::critical(this, tr("Not available"), "Directory is not specified.");
        }
        else if (QMessageBox::Yes ==
                QMessageBox::warning(this, tr("Confirmation"),
                                     tr("This action destroys the data in %1\nDo you want to continue?")
                                     .arg(m_settings->getBenchmarkFile()
                                          .replace("/", QChar(0x2060) + QString("/") + QChar(0x2060))),
                                     QMessageBox::Yes | QMessageBox::No)) {
            m_settings->setFileSize(ui->comboBox_fileSize->currentData().toInt());
            m_benchmark->setRunning(true);
            m_benchmarkThread.start();
        }
    }
}

void MainWindow::benchmarkFailed(const QString &error)
{
    QMessageBox::critical(this, tr("Benchmark Failed"), error);
}

void MainWindow::benchmarkStatusUpdate(const QString &name)
{
    if (m_isBenchmarkThreadRunning)
        setWindowTitle(QLatin1String("%1 - %2").arg(qAppName(), name));
}

void MainWindow::handleResults(QProgressBar *progressBar, const Benchmark::PerformanceResult &result)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<AppSettings::ComparisonField>();

    progressBar->setProperty(metaEnum.valueToKey(AppSettings::MiBPerSec), result.Bandwidth);
    progressBar->setProperty(metaEnum.valueToKey(AppSettings::GiBPerSec), result.Bandwidth / 1024.0);
    progressBar->setProperty(metaEnum.valueToKey(AppSettings::IOPS), result.IOPS);
    progressBar->setProperty(metaEnum.valueToKey(AppSettings::Latency), result.Latency);

    progressBar->setToolTip(
                Global::Instance().getToolTipTemplate().arg(
                    QString::number(result.Bandwidth, 'f', 3),
                    QString::number(result.Bandwidth / 1024, 'f', 3),
                    QString::number(result.IOPS, 'f', 3),
                    QString::number(result.Latency, 'f', 3)
                    )
                );

    updateProgressBar(progressBar);
}

void MainWindow::updateProgressBar(QProgressBar *progressBar)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<AppSettings::ComparisonField>();

    float score = progressBar->property(metaEnum.valueToKey(AppSettings::MiBPerSec)).toFloat();

    float value;

    switch (m_settings->comprasionField) {
    case AppSettings::MiBPerSec:
        progressBar->setFormat(score >= 1000000.0 ? QString::number((int)score) : QString::number(score, 'f', 2));
        break;
    case AppSettings::GiBPerSec:
        value = progressBar->property(metaEnum.valueToKey(AppSettings::GiBPerSec)).toFloat();
        progressBar->setFormat(QString::number(value, 'f', 3));
        break;
    case AppSettings::IOPS:
        value = progressBar->property(metaEnum.valueToKey(AppSettings::IOPS)).toFloat();
        progressBar->setFormat(value >= 1000000.0 ? QString::number((int)value) : QString::number(value, 'f', 2));
        break;
    case AppSettings::Latency:
        value = progressBar->property(metaEnum.valueToKey(AppSettings::Latency)).toFloat();
        progressBar->setFormat(value >= 1000000.0 ? QString::number((int)value) : QString::number(value, 'f', 2));
        break;
    }

    if (m_settings->comprasionField == AppSettings::Latency) {
        progressBar->setValue(value <= 0.0000000001 ? 0 : 100 - 16.666666666666 * log10(value));
    }
    else {
        progressBar->setValue(score <= 0.1 ? 0 : 16.666666666666 * log10(score * 10));
    }
}

void MainWindow::on_pushButton_SEQ_1_clicked()
{
    runOrStopBenchmarkThread();

    if (m_isBenchmarkThreadRunning) {
        runBenchmark(QList<QPair<Benchmark::Type, QProgressBar*>> {
                         { Benchmark::SEQ1M_Q8T1_Read,  ui->readBar_SEQ_1},
                         { Benchmark::SEQ1M_Q8T1_Write, ui->writeBar_SEQ_1 }
                     });
    }
}

void MainWindow::on_pushButton_SEQ_2_clicked()
{
    runOrStopBenchmarkThread();

    if (m_isBenchmarkThreadRunning) {
        runBenchmark(QList<QPair<Benchmark::Type, QProgressBar*>> {
                         { Benchmark::SEQ1M_Q1T1_Read,  ui->readBar_SEQ_2 },
                         { Benchmark::SEQ1M_Q1T1_Write, ui->writeBar_SEQ_2 }
                     });
    }
}

void MainWindow::on_pushButton_RND_1_clicked()
{
    runOrStopBenchmarkThread();

    if (m_isBenchmarkThreadRunning) {
        runBenchmark(QList<QPair<Benchmark::Type, QProgressBar*>> {
                         { Benchmark::RND4K_Q32T16_Read,  ui->readBar_RND_1 },
                         { Benchmark::RND4K_Q32T16_Write, ui->writeBar_RND_1 }
                     });
    }
}

void MainWindow::on_pushButton_RND_2_clicked()
{
    runOrStopBenchmarkThread();

    if (m_isBenchmarkThreadRunning) {
        runBenchmark(QList<QPair<Benchmark::Type, QProgressBar*>> {
                         { Benchmark::RND4K_Q1T1_Read,  ui->readBar_RND_2 },
                         { Benchmark::RND4K_Q1T1_Write, ui->writeBar_RND_2 }
                     });
    }
}

void MainWindow::on_pushButton_All_clicked()
{
    runOrStopBenchmarkThread();

    if (m_isBenchmarkThreadRunning) {
        runBenchmark(QList<QPair<Benchmark::Type, QProgressBar*>> {
                         { Benchmark::SEQ1M_Q8T1_Read,    ui->readBar_SEQ_1  },
                         { Benchmark::SEQ1M_Q1T1_Read,    ui->readBar_SEQ_2  },
                         { Benchmark::RND4K_Q32T16_Read,  ui->readBar_RND_1  },
                         { Benchmark::RND4K_Q1T1_Read,    ui->readBar_RND_2  },
                         { Benchmark::SEQ1M_Q8T1_Write,   ui->writeBar_SEQ_1 },
                         { Benchmark::SEQ1M_Q1T1_Write,   ui->writeBar_SEQ_2 },
                         { Benchmark::RND4K_Q32T16_Write, ui->writeBar_RND_1 },
                         { Benchmark::RND4K_Q1T1_Write,   ui->writeBar_RND_2 }
                     });
    }
}
