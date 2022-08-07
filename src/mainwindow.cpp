#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>
#include <QToolTip>
#include <QMessageBox>
#include <QThread>
#include <QStorageInfo>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QMetaEnum>
#include <QClipboard>
#include <QDate>
#include <QFileDialog>
#include <QTextStream>
#include <QInputDialog>

#include "math.h"
#include "about.h"
#include "settings.h"
#include "diskdriveinfo.h"
#include "global.h"

MainWindow::MainWindow(AppSettings *settings, Benchmark *benchmark, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QActionGroup *localesGroup = new QActionGroup(this);

    QVector<QLocale> locales = { QLocale::English, QLocale::Czech, QLocale::German,
                                 QLocale(QLocale::Spanish, QLocale::Mexico),
                                 QLocale::French, QLocale::Italian, QLocale::Polish,
                                 QLocale(QLocale::Portuguese, QLocale::Brazil),
                                 QLocale::Slovak, QLocale::Russian, QLocale::Ukrainian,
                                 QLocale::Chinese, QLocale::Hindi };

    for (const QLocale &locale : locales) {
        QString langName = locale.nativeLanguageName();
        QAction *lang = new QAction(QString("%1%2").arg(langName[0].toUpper(), langName.mid(1)), this);
        lang->setIcon(QIcon(QStringLiteral(":/icons/flags/%1.svg").arg(locale.name().mid(3))));
        lang->setCheckable(true);
        lang->setData(locale);
        lang->setActionGroup(localesGroup);
        ui->menuLanguage->addAction(lang);

        if (QLocale().name() == locale.name()) lang->setChecked(true);
    }

    connect(localesGroup, SIGNAL(triggered(QAction*)), this, SLOT(localeSelected(QAction*)));

    ui->extraIcon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(QSize(16, 16)));
    ui->extraIcon->setToolTip(tr("The device is encrypted. Performance may drop."));
    ui->extraIcon->setVisible(false);

    statusBar()->hide();

    ui->loopsCount->findChild<QLineEdit*>()->setReadOnly(true);

    m_settings = settings;

    // Default values
    ui->loopsCount->setValue(m_settings->getLoopsCount());

    on_comboBox_ComparisonField_currentIndexChanged(0);

    ui->actionDefault->setProperty("profile", AppSettings::PerformanceProfile::Default);
    ui->actionDefault->setProperty("mixed", false);
    ui->actionPeak_Performance->setProperty("profile", AppSettings::PerformanceProfile::Peak);
    ui->actionPeak_Performance->setProperty("mixed", false);
    ui->actionReal_World_Performance->setProperty("profile", AppSettings::PerformanceProfile::RealWorld);
    ui->actionReal_World_Performance->setProperty("mixed", false);
    ui->actionDefault_Mix->setProperty("profile", AppSettings::PerformanceProfile::Default);
    ui->actionDefault_Mix->setProperty("mixed", true);
    ui->actionPeak_Performance_Mix->setProperty("profile", AppSettings::PerformanceProfile::Peak);
    ui->actionPeak_Performance_Mix->setProperty("mixed", true);
    ui->actionReal_World_Performance_Mix->setProperty("profile", AppSettings::PerformanceProfile::RealWorld);
    ui->actionReal_World_Performance_Mix->setProperty("mixed", true);

    QActionGroup *profilesGroup = new QActionGroup(this);
    ui->actionDefault->setActionGroup(profilesGroup);
    ui->actionPeak_Performance->setActionGroup(profilesGroup);
    ui->actionReal_World_Performance->setActionGroup(profilesGroup);
    ui->actionDefault_Mix->setActionGroup(profilesGroup);
    ui->actionPeak_Performance_Mix->setActionGroup(profilesGroup);
    ui->actionReal_World_Performance_Mix->setActionGroup(profilesGroup);

    connect(profilesGroup, SIGNAL(triggered(QAction*)), this, SLOT(profileSelected(QAction*)));

    profileSelected(ui->actionDefault);

    updateFileSizeList();

    int indexMixRatio = m_settings->getRandomReadPercentage() / 10 - 1;

    for (int i = 1; i <= 9; i++) {
        ui->comboBox_MixRatio->addItem(QStringLiteral("R%1%/W%2%").arg(i * 10).arg((10 - i) * 10));
    }

    ui->comboBox_MixRatio->setCurrentIndex(indexMixRatio);

    m_progressBars << ui->readBar_1 << ui->writeBar_1 << ui->mixBar_1
                   << ui->readBar_2 << ui->writeBar_2 << ui->mixBar_2
                   << ui->readBar_3 << ui->writeBar_3 << ui->mixBar_3
                   << ui->readBar_4 << ui->writeBar_4 << ui->mixBar_4;

    refreshProgressBars();

    updateBenchmarkButtonsContent();

    bool isSomeDeviceMountAsHome = false;
    bool isThereAWritableDir = false;

    // Add each device and its mount point if is writable
    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady() && !storage.isReadOnly()) {
            if (storage.device().indexOf("/dev") != -1) {

                if (storage.rootPath() == QDir::homePath())
                    isSomeDeviceMountAsHome = true;

                addDirectory(storage.rootPath());

                if (!disableDirItemIfIsNotWritable(ui->comboBox_Dirs->count() - 1)
                        && isThereAWritableDir == false) {
                    isThereAWritableDir = true;
                }
            }
        }
    }

    // Add home dir
    if (!isSomeDeviceMountAsHome) {
        QString path = QDir::homePath();

        QStorageInfo storage(path);

        quint64 total = storage.bytesTotal();
        quint64 available = storage.bytesAvailable();

        QStringList volumeInfo = { path, DiskDriveInfo::Instance().getModelName(storage.device()) };

        ui->comboBox_Dirs->insertItem(1,
                    QStringLiteral("%1 %2% (%3)").arg(path)
                    .arg(storage.bytesAvailable() * 100 / total)
                    .arg(formatSize(available, total)),
                    QVariant::fromValue(volumeInfo)
                    );

        if (!disableDirItemIfIsNotWritable(1)
                && isThereAWritableDir == false) {
            isThereAWritableDir = true;
        }
    }

    if (isThereAWritableDir) {
        ui->comboBox_Dirs->setCurrentIndex(1);
    }
    else {
        ui->comboBox_Dirs->setCurrentIndex(-1);
        ui->pushButton_All->setEnabled(false);
        ui->pushButton_Test_1->setEnabled(false);
        ui->pushButton_Test_2->setEnabled(false);
        ui->pushButton_Test_3->setEnabled(false);
        ui->pushButton_Test_4->setEnabled(false);
    }

    // Move Benchmark to another thread and set callbacks
    m_benchmark = benchmark;
    benchmark->moveToThread(&m_benchmarkThread);
    connect(this, &MainWindow::runBenchmark, m_benchmark, &Benchmark::runBenchmark);
    connect(m_benchmark, &Benchmark::runningStateChanged, this, &MainWindow::benchmarkStateChanged);
    connect(m_benchmark, &Benchmark::benchmarkStatusUpdate, this, &MainWindow::benchmarkStatusUpdate);
    connect(m_benchmark, &Benchmark::resultReady, this, &MainWindow::handleResults);
    connect(m_benchmark, &Benchmark::failed, this, &MainWindow::benchmarkFailed);
    connect(m_benchmark, &Benchmark::finished, &m_benchmarkThread, &QThread::quit);

    // About button
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);

    // Settings
    connect(ui->actionQueues_Threads, &QAction::triggered, this, &MainWindow::showSettings);

    connect(ui->actionCopy, &QAction::triggered, this, &MainWindow::copyBenchmarkResult);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveBenchmarkResult);
}

MainWindow::~MainWindow()
{
    m_benchmarkThread.quit();
    m_benchmarkThread.wait();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *)
{
    m_benchmark->stopHelper(); // TEST !

    m_benchmark->setRunning(false);
}

void MainWindow::changeEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::LocaleChange: {
        if (QLocale() == QLocale::AnyLanguage)
            m_settings->setLocale(QLocale::AnyLanguage);
        break;
    }
    case QEvent::LanguageChange: {
        ui->retranslateUi(this);
        updateFileSizeList();
        updateBenchmarkButtonsContent();
        updateLabels();

        QMetaEnum metaEnum = QMetaEnum::fromType<AppSettings::ComparisonField>();

        QLocale locale = QLocale();

        for (auto const& progressBar: m_progressBars) {
            progressBar->setToolTip(
                Global::getToolTipTemplate().arg(
                    locale.toString(progressBar->property(metaEnum.valueToKey(AppSettings::MBPerSec)).toFloat(), 'f', 3),
                    locale.toString(progressBar->property(metaEnum.valueToKey(AppSettings::GBPerSec)).toFloat(), 'f', 3),
                    locale.toString(progressBar->property(metaEnum.valueToKey(AppSettings::IOPS)).toFloat(), 'f', 3),
                    locale.toString(progressBar->property(metaEnum.valueToKey(AppSettings::Latency)).toFloat(), 'f', 3)));
            updateProgressBar(progressBar);
        }

        break;
    }
    default:
        QMainWindow::changeEvent(event);
    }
}

void MainWindow::updateFileSizeList()
{
    int index = ui->comboBox_fileSize->currentIndex();

    if (index == -1) {
        index = ui->comboBox_fileSize->findData(m_settings->getFileSize());
    }

    ui->comboBox_fileSize->clear();

    for (int i = 16; i <= 512; i *= 2) {
        ui->comboBox_fileSize->addItem(QStringLiteral("%1 %2").arg(i).arg(tr("MiB")), i);
    }

    for (int i = 1; i <= 64; i *= 2) {
        ui->comboBox_fileSize->addItem(QStringLiteral("%1 %2").arg(i).arg(tr("GiB")), i * 1024);
    }

    ui->comboBox_fileSize->setCurrentIndex(index);

}

void MainWindow::addDirectory(const QString &path)
{
    if (ui->comboBox_Dirs->findText(path, Qt::MatchContains) != -1)
        return;

    QStorageInfo storage(path);

    quint64 total = storage.bytesTotal();
    quint64 available = storage.bytesAvailable();

    QStringList volumeInfo = { path, DiskDriveInfo::Instance().getModelName(storage.device()) };

    ui->comboBox_Dirs->addItem(
                QStringLiteral("%1 %2% (%3)").arg(path)
                .arg(available * 100 / total)
                .arg(formatSize(available, total)),
                QVariant::fromValue(volumeInfo)
                );
}

void MainWindow::updateBenchmarkButtonsContent()
{
    AppSettings::BenchmarkParams params;

    params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1);
    ui->pushButton_Test_1->setText(QStringLiteral("SEQ%1M\nQ%2T%3").arg(params.BlockSize / 1024)
                                  .arg(params.Queues).arg(params.Threads));

    switch (m_settings->performanceProfile)
    {
    case AppSettings::PerformanceProfile::Default:
        ui->pushButton_Test_1->setToolTip(tr("<h2>Sequential %1 MiB<br/>Queues=%2<br/>Threads=%3</h2>")
                                          .arg(params.BlockSize / 1024).arg(params.Queues).arg(params.Threads));

        params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2);
        ui->pushButton_Test_2->setText(QStringLiteral("SEQ%1M\nQ%2T%3").arg(params.BlockSize / 1024)
                                      .arg(params.Queues).arg(params.Threads));
        ui->pushButton_Test_2->setToolTip(tr("<h2>Sequential %1 MiB<br/>Queues=%2<br/>Threads=%3</h2>")
                                         .arg(params.BlockSize / 1024).arg(params.Queues).arg(params.Threads));

        params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1);
        ui->pushButton_Test_3->setText(QStringLiteral("RND%1K\nQ%2T%3").arg(params.BlockSize)
                                      .arg(params.Queues).arg(params.Threads));
        ui->pushButton_Test_3->setToolTip(tr("<h2>Random %1 KiB<br/>Queues=%2<br/>Threads=%3</h2>")
                                         .arg(params.BlockSize).arg(params.Queues).arg(params.Threads));

        params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2);
        ui->pushButton_Test_4->setText(QStringLiteral("RND%1K\nQ%2T%3").arg(params.BlockSize)
                                      .arg(params.Queues).arg(params.Threads));
        ui->pushButton_Test_4->setToolTip(tr("<h2>Random %1 KiB<br/>Queues=%2<br/>Threads=%3</h2>")
                                         .arg(params.BlockSize).arg(params.Queues).arg(params.Threads));
        break;
    case AppSettings::PerformanceProfile::Peak:
    case AppSettings::PerformanceProfile::RealWorld:
        ui->pushButton_Test_1->setToolTip(tr("<h2>Sequential %1 MiB<br/>Queues=%2<br/>Threads=%3<br/>(%4)</h2>")
                                          .arg(params.BlockSize / 1024).arg(params.Queues).arg(params.Threads).arg(tr("MB/s")));

        params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1);
        ui->pushButton_Test_2->setText(QStringLiteral("RND%1K\nQ%2T%3").arg(params.BlockSize)
                                      .arg(params.Queues).arg(params.Threads));
        ui->pushButton_Test_2->setToolTip(tr("<h2>Random %1 KiB<br/>Queues=%2<br/>Threads=%3<br/>(%4)</h2>")
                                         .arg(params.BlockSize).arg(params.Queues).arg(params.Threads).arg(tr("MB/s")));

        ui->pushButton_Test_3->setText(QStringLiteral("RND%1K\n(IOPS)").arg(params.BlockSize));
        ui->pushButton_Test_3->setToolTip(tr("<h2>Random %1 KiB<br/>Queues=%2<br/>Threads=%3<br/>(%4)</h2>")
                                         .arg(params.BlockSize).arg(params.Queues).arg(params.Threads).arg(tr("IOPS")));

        ui->pushButton_Test_4->setText(QStringLiteral("RND%1K\n(μs)").arg(params.BlockSize));
        ui->pushButton_Test_4->setToolTip(tr("<h2>Random %1 KiB<br/>Queues=%2<br/>Threads=%3<br/>(%4)</h2>")
                                         .arg(params.BlockSize).arg(params.Queues).arg(params.Threads).arg(tr("μs")));
        break;
    }
}

void MainWindow::refreshProgressBars()
{
    QMetaEnum metaEnum = QMetaEnum::fromType<AppSettings::ComparisonField>();

    QLocale locale = QLocale();

    for (auto const& progressBar: m_progressBars) {
        progressBar->setProperty(metaEnum.valueToKey(AppSettings::MBPerSec), 0);
        progressBar->setProperty(metaEnum.valueToKey(AppSettings::GBPerSec), 0);
        progressBar->setProperty(metaEnum.valueToKey(AppSettings::IOPS), 0);
        progressBar->setProperty(metaEnum.valueToKey(AppSettings::Latency), 0);
        progressBar->setValue(0);
        progressBar->setFormat(locale.toString(0., 'f', 2));
        progressBar->setToolTip(Global::getToolTipTemplate().arg(locale.toString(0., 'f', 3), locale.toString(0., 'f', 3), locale.toString(0., 'f', 3), locale.toString(0., 'f', 3)));
    }
}

bool MainWindow::disableDirItemIfIsNotWritable(int index)
{
    QVariant variant = ui->comboBox_Dirs->itemData(index);
    if (variant.canConvert<QStringList>()) {
        QStringList volumeInfo = variant.value<QStringList>();

        if (!QFileInfo(volumeInfo[0]).isWritable()) {
            const QStandardItemModel* model =
                    dynamic_cast<QStandardItemModel*>(ui->comboBox_Dirs->model());
            QStandardItem* item = model->item(index);
            item->setEnabled(false);

            return true;
        }
        else return false;
    }
    else return false;
}

QString MainWindow::formatSize(quint64 available, quint64 total)
{
    QStringList units = { tr("Bytes"), tr("KiB"), tr("MiB"), tr("GiB"), tr("TiB"), tr("PiB") };
    int i;
    double outputAvailable = available;
    double outputTotal = total;
    for (i = 0; i < units.size() - 1; i++) {
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
    updateLabels();

    for (auto const& progressBar: m_progressBars) {
        updateProgressBar(progressBar);
    }
}

void MainWindow::updateLabels()
{
    ui->label_Read->setText(Global::getComparisonLabelTemplate()
                            .arg(tr("Read"), ui->comboBox_ComparisonField->currentText()));

    ui->label_Write->setText(Global::getComparisonLabelTemplate()
                             .arg(tr("Write"), ui->comboBox_ComparisonField->currentText()));

    ui->label_Mix->setText(Global::getComparisonLabelTemplate()
                             .arg(tr("Mix"), ui->comboBox_ComparisonField->currentText()));
}

QString MainWindow::combineOutputTestResult(const QString &name, const QProgressBar *progressBar,
                                         const AppSettings::BenchmarkParams &params)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<AppSettings::ComparisonField>();

    return QString("%1 %2 %3 (Q=%4, T=%5): %6 MB/s [ %7 IOPS] < %8 us>")
           .arg(name)
           .arg(params.BlockSize >= 1024 ? params.BlockSize / 1024 : params.BlockSize)
           .arg(params.BlockSize >= 1024 ? "MiB" : "KiB")
           .arg(QString::number(params.Queues).rightJustified(2, ' '))
           .arg(QString::number(params.Threads).rightJustified(2, ' '))
           .arg(QString::number(
                    progressBar->property(metaEnum.valueToKey(AppSettings::MBPerSec)).toFloat(), 'f', 3)
                .rightJustified(9, ' '))
           .arg(QString::number(
                    progressBar->property(metaEnum.valueToKey(AppSettings::IOPS)).toFloat(), 'f', 1)
                .rightJustified(8, ' '))
           .arg(QString::number(
                    progressBar->property(metaEnum.valueToKey(AppSettings::Latency)).toFloat(), 'f', 2)
                .rightJustified(8, ' '))
           .rightJustified(Global::getOutputColumnsCount(), ' ');
}

QString MainWindow::getTextBenchmarkResult()
{
    QStringList output;

    output << QString("KDiskMark (%1): https://github.com/JonMagon/KDiskMark")
              .arg(qApp->applicationVersion())
              .rightJustified(Global::getOutputColumnsCount(), ' ')
           << QString("Flexible I/O Tester (%1): https://github.com/axboe/fio")
              .arg(m_benchmark->getFIOVersion())
              .rightJustified(Global::getOutputColumnsCount(), ' ')
           << QString("-").repeated(Global::getOutputColumnsCount())
           << "* MB/s = 1,000,000 bytes/s [SATA/600 = 600,000,000 bytes/s]"
           << "* KB = 1000 bytes, KiB = 1024 bytes";

    output << QString()
           << "[Read]"
           << combineOutputTestResult("Sequential", ui->readBar_1,
                                      m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1));
    if (m_settings->performanceProfile == AppSettings::PerformanceProfile::Default)
    output << combineOutputTestResult("Sequential", ui->readBar_2,
                                      m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2));
    output << combineOutputTestResult("Random", ui->readBar_3,
                                      m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1));
    if (m_settings->performanceProfile == AppSettings::PerformanceProfile::Default)
    output << combineOutputTestResult("Random", ui->readBar_4,
                                      m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2));

    output << QString()
           << "[Write]"
           << combineOutputTestResult("Sequential", ui->writeBar_1,
                                      m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1));
    if (m_settings->performanceProfile == AppSettings::PerformanceProfile::Default)
    output << combineOutputTestResult("Sequential", ui->writeBar_2,
                                      m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2));
    output << combineOutputTestResult("Random", ui->writeBar_3,
                                      m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1));
    if (m_settings->performanceProfile == AppSettings::PerformanceProfile::Default)
    output << combineOutputTestResult("Random", ui->writeBar_4,
                                      m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2));

    if (m_settings->isMixed()) {
         output << QString()
                << QString("[Mix] Read %1%/Write %2%")
                   .arg(m_settings->getRandomReadPercentage())
                   .arg(100 - m_settings->getRandomReadPercentage())
                << combineOutputTestResult("Sequential", ui->mixBar_1,
                                           m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1));
         if (m_settings->performanceProfile == AppSettings::PerformanceProfile::Default)
         output << combineOutputTestResult("Sequential", ui->mixBar_2,
                                           m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2));
         output << combineOutputTestResult("Random", ui->mixBar_3,
                                           m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1));
         if (m_settings->performanceProfile == AppSettings::PerformanceProfile::Default)
         output << combineOutputTestResult("Random", ui->mixBar_4,
                                           m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2));
    }

    QString profiles[] = { "Default", "Peak Performance", "Real World Performance" };

    output << QString()
           << QString("Profile: %1%2")
              .arg(profiles[(int)m_settings->performanceProfile]).arg(m_settings->isMixed() ? " [+Mix]" : QString())
           << QString("   Test: %1")
              .arg("%1 %2 (x%3) [Interval: %4 %5]")
              .arg(m_settings->getFileSize() >= 1024 ? m_settings->getFileSize() / 1024 : m_settings->getFileSize())
              .arg(m_settings->getFileSize() >= 1024 ? "GiB" : "MiB")
              .arg(m_settings->getLoopsCount())
              .arg(m_settings->getIntervalTime() >= 60 ? m_settings->getIntervalTime() / 60 : m_settings->getIntervalTime())
              .arg(m_settings->getIntervalTime() >= 60 ? "min" : "sec")
           << QString("   Date: %1 %2")
              .arg(QDate::currentDate().toString("yyyy-MM-dd"))
              .arg(QTime::currentTime().toString("hh:mm:ss"))
           << QString("     OS: %1 %2 [%3 %4]").arg(QSysInfo::productType()).arg(QSysInfo::productVersion())
              .arg(QSysInfo::kernelType()).arg(QSysInfo::kernelVersion());

    return output.join("\n");
}

void MainWindow::copyBenchmarkResult()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(getTextBenchmarkResult());
}

void MainWindow::saveBenchmarkResult()
{
    QString fileName =
            QFileDialog::getSaveFileName(this, QString(),
                                         QStringLiteral("KDM_%1%2.txt").arg(QDate::currentDate().toString("yyyyMMdd"))
                                         .arg(QTime::currentTime().toString("hhmmss")));
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << getTextBenchmarkResult();
            file.close();
        }
    }
}

void MainWindow::on_loopsCount_valueChanged(int arg1)
{
    m_settings->setLoopsCount(arg1);
}

void MainWindow::on_comboBox_MixRatio_currentIndexChanged(int index)
{
    m_settings->setRandomReadPercentage((index + 1) * 10.f);
}

void MainWindow::on_comboBox_Dirs_currentIndexChanged(int index)
{
    if (index == 0) {
        QString dir = QFileDialog::getExistingDirectory(this, QString(), QDir::homePath(),
                                                        QFileDialog::ShowDirsOnly
                                                        | QFileDialog::DontResolveSymlinks
                                                        | QFileDialog::DontUseNativeDialog);
        if (!dir.isNull()) {
            if (QFileInfo(dir).isWritable()) {
                int foundIndex = ui->comboBox_Dirs->findText(dir, Qt::MatchContains);

                if (foundIndex == -1) {
                    addDirectory(dir);
                    ui->comboBox_Dirs->setCurrentIndex(ui->comboBox_Dirs->count() - 1);
                }
                else {
                    ui->comboBox_Dirs->setCurrentIndex(foundIndex);
                }

                return;
            }
            else {
                QMessageBox::critical(this, tr("Bad Directory"), tr("The directory is not writable."));
            }
        }

        ui->comboBox_Dirs->setCurrentIndex(1);
    }
    else {
        QVariant variant = ui->comboBox_Dirs->itemData(index);
        if (variant.canConvert<QStringList>()) {
            QStringList volumeInfo = variant.value<QStringList>();
            m_settings->setDir(volumeInfo[0]);
            ui->deviceModel->setText(volumeInfo[1]);
            ui->extraIcon->setVisible(DiskDriveInfo::Instance().isEncrypted(QStorageInfo(volumeInfo[0]).device()));
        }
    }
}

void MainWindow::localeSelected(QAction* act)
{
    if (!act->data().canConvert<QLocale>()) return;

    m_settings->setLocale(act->data().toLocale());
}

void MainWindow::profileSelected(QAction* act)
{
    bool isMixed = act->property("mixed").toBool();

    m_settings->setMixed(isMixed);

    ui->mixWidget->setVisible(isMixed);
    ui->comboBox_MixRatio->setVisible(isMixed);

    switch (act->property("profile").toInt())
    {
    case AppSettings::PerformanceProfile::Default:
        m_windowTitle = "KDiskMark";
        ui->comboBox_ComparisonField->setVisible(true);
        break;
    case AppSettings::PerformanceProfile::Peak:
        m_windowTitle = "KDiskMark <PEAK>";
        ui->comboBox_ComparisonField->setCurrentIndex(0);
        ui->comboBox_ComparisonField->setVisible(false);
        break;
    case AppSettings::PerformanceProfile::RealWorld:
        m_windowTitle = "KDiskMark <REAL>";
        ui->comboBox_ComparisonField->setCurrentIndex(0);
        ui->comboBox_ComparisonField->setVisible(false);
        break;
    }

    setWindowTitle(m_windowTitle);

    m_settings->performanceProfile = (AppSettings::PerformanceProfile)act->property("profile").toInt();

    int right = isMixed ? ui->mixWidget->geometry().right() : ui->writeWidget->geometry().right();

    ui->targetLayoutWidget->resize(right - ui->targetLayoutWidget->geometry().left(),
                                   ui->targetLayoutWidget->geometry().height());
    ui->commentLayoutWidget->resize(right - ui->commentLayoutWidget->geometry().left(),
                                    ui->commentLayoutWidget->geometry().height());

    setFixedWidth(ui->commentLayoutWidget->geometry().width() + 2 * ui->commentLayoutWidget->geometry().left());

    refreshProgressBars();
    updateBenchmarkButtonsContent();
}

void MainWindow::benchmarkStateChanged(bool state)
{
    if (state) {
        ui->menubar->setEnabled(false);
        ui->loopsCount->setEnabled(false);
        ui->comboBox_fileSize->setEnabled(false);
        ui->comboBox_Dirs->setEnabled(false);
        ui->comboBox_ComparisonField->setEnabled(false);
        ui->comboBox_MixRatio->setEnabled(false);
        ui->pushButton_All->setText(tr("Stop"));
        ui->pushButton_Test_1->setText(tr("Stop"));
        ui->pushButton_Test_2->setText(tr("Stop"));
        ui->pushButton_Test_3->setText(tr("Stop"));
        ui->pushButton_Test_4->setText(tr("Stop"));
    }
    else {
        setWindowTitle(m_windowTitle);
        ui->menubar->setEnabled(true);
        ui->loopsCount->setEnabled(true);
        ui->comboBox_fileSize->setEnabled(true);
        ui->comboBox_Dirs->setEnabled(true);
        ui->comboBox_ComparisonField->setEnabled(true);
        ui->comboBox_MixRatio->setEnabled(true);
        ui->pushButton_All->setEnabled(true);
        ui->pushButton_Test_1->setEnabled(true);
        ui->pushButton_Test_2->setEnabled(true);
        ui->pushButton_Test_3->setEnabled(true);
        ui->pushButton_Test_4->setEnabled(true);
        ui->pushButton_All->setText(tr("All"));
        updateBenchmarkButtonsContent();
    }

    m_isBenchmarkThreadRunning = state;
}

void MainWindow::showAbout()
{
    About about(m_benchmark->getFIOVersion());
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

void MainWindow::inverseBenchmarkThreadRunningState()
{
    if (m_isBenchmarkThreadRunning) {
        m_benchmark->setRunning(false);
        benchmarkStatusUpdate(tr("Stopping..."));
    }
    else {
        if (m_settings->getBenchmarkFile().isNull()) {
            QMessageBox::critical(this, tr("Not available"), tr("Directory is not specified."));
        }
        else if (QMessageBox::Yes ==
                QMessageBox::warning(this, tr("Confirmation"),
                                     tr("This action destroys the data in %1\nDo you want to continue?")
                                     .arg(m_settings->getBenchmarkFile()
                                          .replace("/", QChar(0x2060) + QString("/") + QChar(0x2060))),
                                     QMessageBox::Yes | QMessageBox::No)) {
            m_settings->setFileSize(ui->comboBox_fileSize->currentData().toInt());
            m_settings->setFlushingCacheState(ui->actionFlush_Pagecache->isChecked());
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
        setWindowTitle(QString("%1 - %2").arg(m_windowTitle, name));
}

void MainWindow::handleResults(QProgressBar *progressBar, const Benchmark::PerformanceResult &result)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<AppSettings::ComparisonField>();

    progressBar->setProperty(metaEnum.valueToKey(AppSettings::MBPerSec), result.Bandwidth);
    progressBar->setProperty(metaEnum.valueToKey(AppSettings::GBPerSec), result.Bandwidth / 1000);
    progressBar->setProperty(metaEnum.valueToKey(AppSettings::IOPS), result.IOPS);
    progressBar->setProperty(metaEnum.valueToKey(AppSettings::Latency), result.Latency);

    QLocale locale = QLocale();

    progressBar->setToolTip(
                Global::getToolTipTemplate().arg(
                    locale.toString(result.Bandwidth, 'f', 3),
                    locale.toString(result.Bandwidth / 1000, 'f', 3),
                    locale.toString(result.IOPS, 'f', 3),
                    locale.toString(result.Latency, 'f', 3)
                    )
                );

    updateProgressBar(progressBar);
}

void MainWindow::updateProgressBar(QProgressBar *progressBar)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<AppSettings::ComparisonField>();

    float score = progressBar->property(metaEnum.valueToKey(AppSettings::MBPerSec)).toFloat();

    float value;

    AppSettings::ComparisonField comparisonField = AppSettings::MBPerSec;

    switch (m_settings->performanceProfile) {
    case AppSettings::PerformanceProfile::Peak:
    case AppSettings::PerformanceProfile::RealWorld:
        if (progressBar == ui->readBar_3 || progressBar == ui->writeBar_3 || progressBar == ui->mixBar_3) {
            comparisonField = AppSettings::IOPS;
        }
        else if (progressBar == ui->readBar_4 || progressBar == ui->writeBar_4 || progressBar == ui->mixBar_4) {
            comparisonField = AppSettings::Latency;
        }
        break;
    default:
        comparisonField = m_settings->comprasionField;
        break;
    }

    QLocale locale = QLocale();

    switch (comparisonField) {
    case AppSettings::MBPerSec:
        progressBar->setFormat(score >= 1000000.0 ? locale.toString((int)score) : locale.toString(score, 'f', 2));
        break;
    case AppSettings::GBPerSec:
        value = progressBar->property(metaEnum.valueToKey(AppSettings::GBPerSec)).toFloat();
        progressBar->setFormat(locale.toString(value, 'f', 3));
        break;
    case AppSettings::IOPS:
        value = progressBar->property(metaEnum.valueToKey(AppSettings::IOPS)).toFloat();
        progressBar->setFormat(value >= 1000000.0 ? locale.toString((int)value) : locale.toString(value, 'f', 2));
        break;
    case AppSettings::Latency:
        value = progressBar->property(metaEnum.valueToKey(AppSettings::Latency)).toFloat();
        progressBar->setFormat(value >= 1000000.0 ? locale.toString((int)value) : locale.toString(value, 'f', 2));
        break;
    }

    if (comparisonField == AppSettings::Latency) {
        progressBar->setValue(value <= 0.0000000001 ? 0 : 100 - 16.666666666666 * log10(value));
    }
    else {
        progressBar->setValue(score <= 0.1 ? 0 : 16.666666666666 * log10(score * 10));
    }
}

bool MainWindow::runCombinedRandomTest()
{
    if (m_settings->performanceProfile != AppSettings::PerformanceProfile::Default) {
        QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> set {
            { Benchmark::RND_1_Read,  { ui->readBar_2,  ui->readBar_3,  ui->readBar_4  } },
            { Benchmark::RND_1_Write, { ui->writeBar_2, ui->writeBar_3, ui->writeBar_4 } }
        };

        if (m_settings->isMixed()) {
            set << QPair<Benchmark::Type, QVector<QProgressBar*>>
            { Benchmark::RND_1_Mix,   {  ui->mixBar_2,  ui->mixBar_3,   ui->mixBar_4   } };
        }

        runBenchmark(set);

        return true;
    }

    return false;
}

void MainWindow::on_pushButton_Test_1_clicked()
{
    inverseBenchmarkThreadRunningState();

    if (m_isBenchmarkThreadRunning) {
        QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> set {
            { Benchmark::SEQ_1_Read,  { ui->readBar_1  } },
            { Benchmark::SEQ_1_Write, { ui->writeBar_1 } }
        };

        if (m_settings->isMixed()) {
            set << QPair<Benchmark::Type, QVector<QProgressBar*>>
            { Benchmark::SEQ_1_Mix,   { ui->mixBar_1   } };
        }

        runBenchmark(set);
    }
}

void MainWindow::on_pushButton_Test_2_clicked()
{
    inverseBenchmarkThreadRunningState();

    if (m_isBenchmarkThreadRunning) {
        if (runCombinedRandomTest()) return;

        QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> set {
            { Benchmark::SEQ_2_Read,  { ui->readBar_2  } },
            { Benchmark::SEQ_2_Write, { ui->writeBar_2 } }
        };

        if (m_settings->isMixed()) {
            set << QPair<Benchmark::Type, QVector<QProgressBar*>>
            { Benchmark::SEQ_2_Mix,   { ui->mixBar_2   } };
        }

        runBenchmark(set);
    }
}

void MainWindow::on_pushButton_Test_3_clicked()
{
    inverseBenchmarkThreadRunningState();

    if (m_isBenchmarkThreadRunning) {
        if (runCombinedRandomTest()) return;

        QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> set {
            { Benchmark::RND_1_Read,  { ui->readBar_3  } },
            { Benchmark::RND_1_Write, { ui->writeBar_3 } }
        };

        if (m_settings->isMixed()) {
            set << QPair<Benchmark::Type, QVector<QProgressBar*>>
            { Benchmark::RND_1_Mix,   { ui->mixBar_3   } };
        }

        runBenchmark(set);
    }
}

void MainWindow::on_pushButton_Test_4_clicked()
{
    inverseBenchmarkThreadRunningState();

    if (m_isBenchmarkThreadRunning) {
        if (runCombinedRandomTest()) return;

        QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> set {
            { Benchmark::RND_2_Read,  { ui->readBar_4  } },
            { Benchmark::RND_2_Write, { ui->writeBar_4 } }
        };

        if (m_settings->isMixed()) {
            set << QPair<Benchmark::Type, QVector<QProgressBar*>>
            { Benchmark::RND_2_Mix,   { ui->mixBar_4   } };
        }

        runBenchmark(set);
    }
}

void MainWindow::on_pushButton_All_clicked()
{
    if (m_benchmark->startHelper())
        QMessageBox::information(this, "TEST", "ON");
    else
        QMessageBox::warning(this, "TEST", "FAIL startHelper");

    //m_benchmark->stopHelper();
    //QMessageBox::information(this, "TEST", "OFF");

    return;

    // TEST !

    inverseBenchmarkThreadRunningState();

    if (m_isBenchmarkThreadRunning) {
        QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> set;

        if (m_settings->performanceProfile == AppSettings::PerformanceProfile::Default) {
            set << QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> {
                { Benchmark::SEQ_1_Read,  { ui->readBar_1  } },
                { Benchmark::SEQ_2_Read,  { ui->readBar_2  } },
                { Benchmark::RND_1_Read,  { ui->readBar_3  } },
                { Benchmark::RND_2_Read,  { ui->readBar_4  } },
                { Benchmark::SEQ_1_Write, { ui->writeBar_1 } },
                { Benchmark::SEQ_2_Write, { ui->writeBar_2 } },
                { Benchmark::RND_1_Write, { ui->writeBar_3 } },
                { Benchmark::RND_2_Write, { ui->writeBar_4 } }
            };

            if (m_settings->isMixed()) {
                set << QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> {
                { Benchmark::SEQ_1_Mix,   { ui->mixBar_1   } },
                { Benchmark::SEQ_2_Mix,   { ui->mixBar_2   } },
                { Benchmark::RND_1_Mix,   { ui->mixBar_3   } },
                { Benchmark::RND_2_Mix,   { ui->mixBar_4   } }
            };
            }
        }
        else {
            set << QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> {
                { Benchmark::SEQ_1_Read,  { ui->readBar_1  } },
                { Benchmark::RND_1_Read,  { ui->readBar_2,  ui->readBar_3,  ui->readBar_4  } },
                { Benchmark::SEQ_1_Write, { ui->writeBar_1 } },
                { Benchmark::RND_1_Write, { ui->writeBar_2, ui->writeBar_3, ui->writeBar_4 } }
            };

            if (m_settings->isMixed()) {
                set << QList<QPair<Benchmark::Type, QVector<QProgressBar*>>> {
                { Benchmark::SEQ_1_Mix,   { ui->mixBar_1   } },
                { Benchmark::RND_1_Mix,   {  ui->mixBar_2,   ui->mixBar_3,   ui->mixBar_4  } }
            };
            }
        }

        runBenchmark(set);
    }
}
