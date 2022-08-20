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
#include <QAbstractItemView>
#include <QStyleFactory>

#include "math.h"
#include "about.h"
#include "settings.h"
#include "diskdriveinfo.h"
#include "global.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_benchmark(new Benchmark(new AppSettings))
{
    ui->setupUi(this);

    const AppSettings settings;

    m_settings = new AppSettings;

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

    // Default values
    ui->loopsCount->setValue(settings.getLoopsCount());

    on_comboBox_ComparisonField_currentIndexChanged(0);

    ui->actionDefault->setProperty("profile", Global::PerformanceProfile::Default);
    ui->actionDefault->setProperty("mixed", false);
    ui->actionPeak_Performance->setProperty("profile", Global::PerformanceProfile::Peak);
    ui->actionPeak_Performance->setProperty("mixed", false);
    ui->actionReal_World_Performance->setProperty("profile", Global::PerformanceProfile::RealWorld);
    ui->actionReal_World_Performance->setProperty("mixed", false);
    ui->actionDemo->setProperty("profile", Global::PerformanceProfile::Demo);
    ui->actionDemo->setProperty("mixed", false);
    ui->actionDefault_Mix->setProperty("profile", Global::PerformanceProfile::Default);
    ui->actionDefault_Mix->setProperty("mixed", true);
    ui->actionPeak_Performance_Mix->setProperty("profile", Global::PerformanceProfile::Peak);
    ui->actionPeak_Performance_Mix->setProperty("mixed", true);
    ui->actionReal_World_Performance_Mix->setProperty("profile", Global::PerformanceProfile::RealWorld);
    ui->actionReal_World_Performance_Mix->setProperty("mixed", true);

    QActionGroup *profilesGroup = new QActionGroup(this);
    ui->actionDefault->setActionGroup(profilesGroup);
    ui->actionPeak_Performance->setActionGroup(profilesGroup);
    ui->actionReal_World_Performance->setActionGroup(profilesGroup);
    ui->actionDemo->setActionGroup(profilesGroup);
    ui->actionDefault_Mix->setActionGroup(profilesGroup);
    ui->actionPeak_Performance_Mix->setActionGroup(profilesGroup);
    ui->actionReal_World_Performance_Mix->setActionGroup(profilesGroup);

    connect(profilesGroup, SIGNAL(triggered(QAction*)), this, SLOT(profileSelected(QAction*)));

    ui->actionFlush_Pagecache->setChecked(settings.getFlusingCacheState());

    profileSelected(ui->actionDefault);

    updateFileSizeList();

    int indexMixRatio = settings.getRandomReadPercentage() / 10 - 1;

    for (int i = 1; i <= 9; i++) {
        ui->comboBox_MixRatio->addItem(QStringLiteral("R%1%/W%2%").arg(i * 10).arg((10 - i) * 10));
    }

    ui->comboBox_MixRatio->setCurrentIndex(indexMixRatio);

    m_progressBars << ui->readBar_1 << ui->writeBar_1 << ui->mixBar_1
                   << ui->readBar_2 << ui->writeBar_2 << ui->mixBar_2
                   << ui->readBar_3 << ui->writeBar_3 << ui->mixBar_3
                   << ui->readBar_4 << ui->writeBar_4 << ui->mixBar_4
                   << ui->readBar_Demo << ui->writeBar_Demo;

    QStyle *progressBarStyle = QStyleFactory::create("Fusion");
    for (auto const& progressBar: m_progressBars) {
        progressBar->setStyle(progressBarStyle);
    }

    refreshProgressBars();

    updateBenchmarkButtonsContent();

    // Set callbacks
    connect(m_benchmark, &Benchmark::runningStateChanged, this, &MainWindow::benchmarkStateChanged);
    connect(m_benchmark, &Benchmark::benchmarkStatusUpdate, this, &MainWindow::benchmarkStatusUpdate);
    connect(m_benchmark, &Benchmark::resultReady, this, &MainWindow::handleResults);
    connect(m_benchmark, &Benchmark::failed, this, &MainWindow::benchmarkFailed);

    // About button
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);

    // Settings
    connect(ui->actionQueues_Threads, &QAction::triggered, this, &MainWindow::showSettings);

    connect(ui->actionCopy, &QAction::triggered, this, &MainWindow::copyBenchmarkResult);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveBenchmarkResult);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *)
{
    auto stopHelper = [&] () { m_benchmark->stopHelper(); };
    if (m_benchmark->isRunning()) {
        connect(m_benchmark, &Benchmark::finished, this, stopHelper);
        m_benchmark->setRunning(false);
    }
    else {
        stopHelper();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::LocaleChange: {
        if (const QLocale locale = AppSettings().locale(); locale == AppSettings::defaultLocale())
            AppSettings::applyLocale(locale);
        break;
    }
    case QEvent::LanguageChange: {
        ui->retranslateUi(this);
        updateFileSizeList();
        updateBenchmarkButtonsContent();
        updateLabels();

        QMetaEnum metaEnum = QMetaEnum::fromType<Benchmark::ComparisonField>();

        QLocale locale = QLocale();

        for (auto const& progressBar: m_progressBars) {
            if (!progressBar->property("Demo").toBool())
                progressBar->setToolTip(
                    Global::getToolTipTemplate().arg(
                        locale.toString(progressBar->property(metaEnum.valueToKey(Benchmark::MBPerSec)).toFloat(), 'f', 3),
                        locale.toString(progressBar->property(metaEnum.valueToKey(Benchmark::GBPerSec)).toFloat(), 'f', 3),
                        locale.toString(progressBar->property(metaEnum.valueToKey(Benchmark::IOPS)).toFloat(), 'f', 3),
                        locale.toString(progressBar->property(metaEnum.valueToKey(Benchmark::Latency)).toFloat(), 'f', 3)));
            updateProgressBar(progressBar);
        }

        break;
    }
    default:
        QMainWindow::changeEvent(event);
    }
}

void MainWindow::on_refreshStoragesButton_clicked()
{
    if (!m_benchmark->listStorages()) {
        QMessageBox::critical(this, tr("Access Denied"), tr("Failed to retrieve storage list."));
        return;
    }

    QString temp;

    QVariant variant = ui->comboBox_Storages->currentData();
    if (variant.canConvert<QStringList>())
        temp = variant.value<QStringList>()[0];

    ui->comboBox_Storages->clear();

    for (Benchmark::Storage storage : m_benchmark->storages) {
        QStringList volumeInfo = { storage.path, DiskDriveInfo::Instance().getModelName(QStorageInfo(storage.path).device()) };

        ui->comboBox_Storages->addItem(
                    QStringLiteral("%1 %2% (%3)").arg(storage.path)
                    .arg(storage.bytesOccupied * 100 / storage.bytesTotal)
                    .arg(formatSize(storage.bytesOccupied, storage.bytesTotal)),
                    QVariant::fromValue(volumeInfo)
                    );
    }

    if (!temp.isEmpty()) {
        int foundIndex = ui->comboBox_Storages->findText(temp, Qt::MatchContains);
        if (foundIndex != -1) ui->comboBox_Storages->setCurrentIndex(foundIndex);
    }

    // Resize items popup
    resizeComboBoxItemsPopup(ui->comboBox_Storages);
}

void MainWindow::updateFileSizeList()
{
    const AppSettings settings;

    int fileSize = settings.getFileSize();

    ui->comboBox_fileSize->clear();

    for (int i = 16; i <= 512; i *= 2) {
        ui->comboBox_fileSize->addItem(QStringLiteral("%1 %2").arg(i).arg(tr("MiB")), i);
    }

    for (int i = 1; i <= 64; i *= 2) {
        ui->comboBox_fileSize->addItem(QStringLiteral("%1 %2").arg(i).arg(tr("GiB")), i * 1024);
    }

    ui->comboBox_fileSize->setCurrentIndex(ui->comboBox_fileSize->findData(fileSize));

    resizeComboBoxItemsPopup(ui->comboBox_fileSize);
}

void MainWindow::on_comboBox_fileSize_currentIndexChanged(int index)
{
    if (index != -1) {
        AppSettings settings;
        settings.setFileSize(ui->comboBox_fileSize->currentData().toInt());
    }
}

void MainWindow::resizeComboBoxItemsPopup(QComboBox *combobox)
{
    int maxWidth = 0;
    QFontMetrics fontMetrics(combobox->font());
    for (int i = 0; i < combobox->count(); i++)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        int width = fontMetrics.horizontalAdvance(combobox->itemText(i));
#else
        int width = fontMetrics.width(combobox->itemText(i));
#endif
        if (width > maxWidth)
            maxWidth = width;
    }

    QAbstractItemView *view = combobox->view();
    if (view->minimumWidth() < maxWidth)
        view->setMinimumWidth(maxWidth + view->autoScrollMargin() + combobox->style()->pixelMetric(QStyle::PM_ScrollBarExtent));
}

void MainWindow::on_actionFlush_Pagecache_triggered(bool checked)
{
    AppSettings settings;
    settings.setFlushingCacheState(checked);
}

void MainWindow::updateBenchmarkButtonsContent()
{
    const AppSettings settings;

    Global::BenchmarkParams params;

    params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_1, m_benchmark->performanceProfile);
    ui->pushButton_Test_1->setText(Global::getBenchmarkButtonText(params));

    switch (m_benchmark->performanceProfile)
    {
    case Global::PerformanceProfile::Default:
        ui->pushButton_Test_1->setToolTip(Global::getBenchmarkButtonToolTip(params));

        params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_2);
        ui->pushButton_Test_2->setText(Global::getBenchmarkButtonText(params));
        ui->pushButton_Test_2->setToolTip(Global::getBenchmarkButtonToolTip(params));

        params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_3);
        ui->pushButton_Test_3->setText(Global::getBenchmarkButtonText(params));
        ui->pushButton_Test_3->setToolTip(Global::getBenchmarkButtonToolTip(params));

        params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_4);
        ui->pushButton_Test_4->setText(Global::getBenchmarkButtonText(params));
        ui->pushButton_Test_4->setToolTip(Global::getBenchmarkButtonToolTip(params));
        break;
    case Global::PerformanceProfile::Peak:
    case Global::PerformanceProfile::RealWorld:
        ui->pushButton_Test_1->setToolTip(Global::getBenchmarkButtonToolTip(params, true).arg(tr("MB/s")));

        params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_2, m_benchmark->performanceProfile);
        ui->pushButton_Test_2->setText(Global::getBenchmarkButtonText(params));
        ui->pushButton_Test_2->setToolTip(Global::getBenchmarkButtonToolTip(params, true).arg(tr("MB/s")));

        ui->pushButton_Test_3->setText(Global::getBenchmarkButtonText(params, tr("IOPS")));
        ui->pushButton_Test_3->setToolTip(Global::getBenchmarkButtonToolTip(params, true).arg(tr("IOPS")));

        ui->pushButton_Test_4->setText(Global::getBenchmarkButtonText(params, tr("μs")));
        ui->pushButton_Test_4->setToolTip(Global::getBenchmarkButtonToolTip(params, true).arg(tr("μs")));
        break;
    case Global::PerformanceProfile::Demo:
        ui->label_Demo->setText(QStringLiteral("%1 %2 %3, Q=%4, T=%5")
                                .arg(params.Pattern == Global::BenchmarkIOPattern::SEQ ? QStringLiteral("SEQ") : QStringLiteral("RND"))
                                .arg(params.BlockSize >= 1024 ? params.BlockSize / 1024 : params.BlockSize)
                                .arg(params.BlockSize >= 1024 ? tr("MiB") : tr("KiB"))
                                .arg(params.Queues).arg(params.Threads));
        break;
    }
}

void MainWindow::refreshProgressBars()
{
    QMetaEnum metaEnum = QMetaEnum::fromType<Benchmark::ComparisonField>();

    QLocale locale = QLocale();

    for (auto const& progressBar: m_progressBars) {
        progressBar->setProperty(metaEnum.valueToKey(Benchmark::MBPerSec), 0);
        progressBar->setProperty(metaEnum.valueToKey(Benchmark::GBPerSec), 0);
        progressBar->setProperty(metaEnum.valueToKey(Benchmark::IOPS), 0);
        progressBar->setProperty(metaEnum.valueToKey(Benchmark::Latency), 0);
        progressBar->setValue(0);
        progressBar->setFormat(locale.toString(0., 'f', progressBar->property("Demo").toBool() ? 1 : 2));
        if (!progressBar->property("Demo").toBool())
        progressBar->setToolTip(
                    Global::getToolTipTemplate().arg(
                        locale.toString(0., 'f', 3),
                        locale.toString(0., 'f', 3),
                        locale.toString(0., 'f', 3),
                        locale.toString(0., 'f', 3)
                        )
                    );
    }
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
    m_benchmark->comprasionField = Benchmark::ComparisonField(index);
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

    ui->label_Unit_Read_Demo->setText(ui->comboBox_ComparisonField->currentText());
    ui->label_Unit_Write_Demo->setText(ui->comboBox_ComparisonField->currentText());
}

QString MainWindow::combineOutputTestResult(const QString &name, const QProgressBar *progressBar,
                                            const Global::BenchmarkParams &params)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<Benchmark::ComparisonField>();

    return QString("%1 %2 %3 (Q=%4, T=%5): %6 MB/s [ %7 IOPS] < %8 us>")
           .arg(name)
           .arg(params.BlockSize >= 1024 ? params.BlockSize / 1024 : params.BlockSize)
           .arg(params.BlockSize >= 1024 ? "MiB" : "KiB")
           .arg(QString::number(params.Queues).rightJustified(2, ' '))
           .arg(QString::number(params.Threads).rightJustified(2, ' '))
           .arg(QString::number(
                    progressBar->property(metaEnum.valueToKey(Benchmark::MBPerSec)).toFloat(), 'f', 3)
                .rightJustified(9, ' '))
           .arg(QString::number(
                    progressBar->property(metaEnum.valueToKey(Benchmark::IOPS)).toFloat(), 'f', 1)
                .rightJustified(8, ' '))
           .arg(QString::number(
                    progressBar->property(metaEnum.valueToKey(Benchmark::Latency)).toFloat(), 'f', 2)
                .rightJustified(8, ' '))
           .rightJustified(Global::getOutputColumnsCount(), ' ');
}

QString MainWindow::getTextBenchmarkResult()
{/*
    const AppSettings settings;

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
                                      settings.getBenchmarkParams(Global::BenchmarkTest::SEQ_1));
    if (m_benchmark->performanceProfile == Benchmark::PerformanceProfile::Default)
    output << combineOutputTestResult("Sequential", ui->readBar_2,
                                      settings.getBenchmarkParams(Global::BenchmarkTest::SEQ_2));
    output << combineOutputTestResult("Random", ui->readBar_3,
                                      settings.getBenchmarkParams(Global::BenchmarkTest::RND_1));
    if (m_benchmark->performanceProfile == Benchmark::PerformanceProfile::Default)
    output << combineOutputTestResult("Random", ui->readBar_4,
                                      settings.getBenchmarkParams(Global::BenchmarkTest::RND_2));

    output << QString()
           << "[Write]"
           << combineOutputTestResult("Sequential", ui->writeBar_1,
                                      settings.getBenchmarkParams(Global::BenchmarkTest::SEQ_1));
    if (m_benchmark->performanceProfile == Benchmark::PerformanceProfile::Default)
    output << combineOutputTestResult("Sequential", ui->writeBar_2,
                                      settings.getBenchmarkParams(Global::BenchmarkTest::SEQ_2));
    output << combineOutputTestResult("Random", ui->writeBar_3,
                                      settings.getBenchmarkParams(Global::BenchmarkTest::RND_1));
    if (m_benchmark->performanceProfile == Benchmark::PerformanceProfile::Default)
    output << combineOutputTestResult("Random", ui->writeBar_4,
                                      settings.getBenchmarkParams(Global::BenchmarkTest::RND_2));

    if (m_benchmark->isMixed()) {
         output << QString()
                << QString("[Mix] Read %1%/Write %2%")
                   .arg(settings.getRandomReadPercentage())
                   .arg(100 - settings.getRandomReadPercentage())
                << combineOutputTestResult("Sequential", ui->mixBar_1,
                                           settings.getBenchmarkParams(Global::BenchmarkTest::SEQ_1));
         if (m_benchmark->performanceProfile == Benchmark::PerformanceProfile::Default)
         output << combineOutputTestResult("Sequential", ui->mixBar_2,
                                           settings.getBenchmarkParams(Global::BenchmarkTest::SEQ_2));
         output << combineOutputTestResult("Random", ui->mixBar_3,
                                           settings.getBenchmarkParams(Global::BenchmarkTest::RND_1));
         if (m_benchmark->performanceProfile == Benchmark::PerformanceProfile::Default)
         output << combineOutputTestResult("Random", ui->mixBar_4,
                                           settings.getBenchmarkParams(Global::BenchmarkTest::RND_2));
    }

    QString profiles[] = { "Default", "Peak Performance", "Real World Performance" };

    output << QString()
           << QString("Profile: %1%2")
              .arg(profiles[(int)m_benchmark->performanceProfile]).arg(m_benchmark->isMixed() ? " [+Mix]" : QString())
           << QString("   Test: %1")
              .arg("%1 %2 (x%3) [Interval: %4 %5]")
              .arg(settings.getFileSize() >= 1024 ? settings.getFileSize() / 1024 : settings.getFileSize())
              .arg(settings.getFileSize() >= 1024 ? "GiB" : "MiB")
              .arg(settings.getLoopsCount())
              .arg(settings.getIntervalTime() >= 60 ? settings.getIntervalTime() / 60 : settings.getIntervalTime())
              .arg(settings.getIntervalTime() >= 60 ? "min" : "sec")
           << QString("   Date: %1 %2")
              .arg(QDate::currentDate().toString("yyyy-MM-dd"))
              .arg(QTime::currentTime().toString("hh:mm:ss"))
           << QString("     OS: %1 %2 [%3 %4]").arg(QSysInfo::productType()).arg(QSysInfo::productVersion())
              .arg(QSysInfo::kernelType()).arg(QSysInfo::kernelVersion());

    return output.join("\n");*/
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
    AppSettings settings;
    settings.setLoopsCount(arg1);
}

void MainWindow::on_comboBox_MixRatio_currentIndexChanged(int index)
{
    AppSettings settings;
    settings.setRandomReadPercentage((index + 1) * 10.f);
}

void MainWindow::on_comboBox_Storages_currentIndexChanged(int index)
{
    QVariant variant = ui->comboBox_Storages->itemData(index);
    if (variant.canConvert<QStringList>()) {
        QStringList volumeInfo = variant.value<QStringList>();
        m_benchmark->setDir(volumeInfo[0]);
        ui->deviceModel->setText(volumeInfo[1]);
        ui->extraIcon->setVisible(DiskDriveInfo::Instance().isEncrypted(QStorageInfo(volumeInfo[0]).device()));
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

    m_benchmark->setMixed(isMixed);

    ui->mixWidget->setVisible(isMixed);
    ui->comboBox_MixRatio->setVisible(isMixed);

    switch (act->property("profile").toInt())
    {
    case Global::PerformanceProfile::Default:
        m_windowTitle = "KDiskMark";
        ui->comboBox_ComparisonField->setVisible(true);
        break;
    case Global::PerformanceProfile::Peak:
        m_windowTitle = "KDiskMark <PEAK>";
        ui->comboBox_ComparisonField->setCurrentIndex(0);
        ui->comboBox_ComparisonField->setVisible(false);
        break;
    case Global::PerformanceProfile::RealWorld:
        m_windowTitle = "KDiskMark <REAL>";
        ui->comboBox_ComparisonField->setCurrentIndex(0);
        ui->comboBox_ComparisonField->setVisible(false);
        break;
    case Global::PerformanceProfile::Demo:
        m_windowTitle = "KDiskMark <DEMO>";
        break;
    }



    setWindowTitle(m_windowTitle);

    m_benchmark->performanceProfile = (Global::PerformanceProfile)act->property("profile").toInt();

    ui->stackedWidget->setCurrentIndex(m_benchmark->performanceProfile == Global::PerformanceProfile::Demo ? 1 : 0);

    int right = (isMixed ? ui->mixWidget->geometry().right() : ui->writeWidget->geometry().right()) + ui->stackedWidget->geometry().x();

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
        ui->comboBox_Storages->setEnabled(false);
        ui->refreshStoragesButton->setEnabled(false);
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
        ui->pushButton_All->setEnabled(true);
        ui->pushButton_Test_1->setEnabled(true);
        ui->pushButton_Test_2->setEnabled(true);
        ui->pushButton_Test_3->setEnabled(true);
        ui->pushButton_Test_4->setEnabled(true);
        ui->menubar->setEnabled(true);
        ui->loopsCount->setEnabled(true);
        ui->comboBox_fileSize->setEnabled(true);
        ui->comboBox_Storages->setEnabled(true);
        ui->refreshStoragesButton->setEnabled(true);
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
}

void MainWindow::showAbout()
{
    About about(m_benchmark->getFIOVersion());
    about.setFixedSize(about.size());
    about.exec();
}

void MainWindow::showSettings()
{
    Settings settings;
    settings.setFixedSize(settings.size());
    settings.exec();

    updateBenchmarkButtonsContent();
}

void MainWindow::defineBenchmark(std::function<void()> bodyFunc)
{
    AppSettings settings;

    if (m_benchmark->isRunning()) {
        benchmarkStatusUpdate(tr("Stopping..."));
        ui->pushButton_All->setEnabled(false);
        ui->pushButton_Test_1->setEnabled(false);
        ui->pushButton_Test_2->setEnabled(false);
        ui->pushButton_Test_3->setEnabled(false);
        ui->pushButton_Test_4->setEnabled(false);
        m_benchmark->setRunning(false);
    }
    else {
        if (m_benchmark->getBenchmarkFile().isNull()) {
            QMessageBox::critical(this, tr("Not available"), tr("Directory is not specified."));
        }
        else if (QMessageBox::Yes ==
                QMessageBox::warning(this, tr("Confirmation"),
                                     tr("This action destroys the data in %1\nDo you want to continue?")
                                     .arg(m_benchmark->getBenchmarkFile()
                                          .replace("/", QChar(0x2060) + QString("/") + QChar(0x2060))),
                                     QMessageBox::Yes | QMessageBox::No)) {
            bodyFunc();
        }
    }
}

void MainWindow::benchmarkFailed(const QString &error)
{
    QMessageBox::critical(this, tr("Benchmark Failed"), error);
}

void MainWindow::benchmarkStatusUpdate(const QString &name)
{
    setWindowTitle(QString("%1 - %2").arg(m_windowTitle, name));
}

void MainWindow::handleResults(QProgressBar *progressBar, const Benchmark::PerformanceResult &result)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<Benchmark::ComparisonField>();

    progressBar->setProperty(metaEnum.valueToKey(Benchmark::MBPerSec), result.Bandwidth);
    progressBar->setProperty(metaEnum.valueToKey(Benchmark::GBPerSec), result.Bandwidth / 1000);
    progressBar->setProperty(metaEnum.valueToKey(Benchmark::IOPS), result.IOPS);
    progressBar->setProperty(metaEnum.valueToKey(Benchmark::Latency), result.Latency);

    if (!progressBar->property("Demo").toBool()) {
        QLocale locale = QLocale();

        progressBar->setToolTip(
                    Global::getToolTipTemplate().arg(
                        locale.toString(result.Bandwidth, 'f', 3),
                        locale.toString(result.Bandwidth / 1000, 'f', 3),
                        locale.toString(result.IOPS, 'f', 3),
                        locale.toString(result.Latency, 'f', 3)
                        )
                    );
    }

    updateProgressBar(progressBar);
}

void MainWindow::updateProgressBar(QProgressBar *progressBar)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<Benchmark::ComparisonField>();

    float score = progressBar->property(metaEnum.valueToKey(Benchmark::MBPerSec)).toFloat();

    float value;

    Benchmark::ComparisonField comparisonField = Benchmark::MBPerSec;

    switch (m_benchmark->performanceProfile) {
    case Global::PerformanceProfile::Peak:
    case Global::PerformanceProfile::RealWorld:
        if (progressBar == ui->readBar_3 || progressBar == ui->writeBar_3 || progressBar == ui->mixBar_3) {
            comparisonField = Benchmark::IOPS;
        }
        else if (progressBar == ui->readBar_4 || progressBar == ui->writeBar_4 || progressBar == ui->mixBar_4) {
            comparisonField = Benchmark::Latency;
        }
        break;
    default:
        comparisonField = m_benchmark->comprasionField;
        break;
    }

    QLocale locale = QLocale();

    switch (comparisonField) {
    case Benchmark::MBPerSec:
        progressBar->setFormat(score >= 1000000.0 ? locale.toString((int)score) : locale.toString(score, 'f', progressBar->property("Demo").toBool() ? 1 : 2));
        break;
    case Benchmark::GBPerSec:
        value = progressBar->property(metaEnum.valueToKey(Benchmark::GBPerSec)).toFloat();
        progressBar->setFormat(locale.toString(value, 'f', progressBar->property("Demo").toBool() ? 1 : 3));
        break;
    case Benchmark::IOPS:
        value = progressBar->property(metaEnum.valueToKey(Benchmark::IOPS)).toFloat();
        progressBar->setFormat(value >= 1000000.0 ? locale.toString((int)value) : locale.toString(value, 'f', progressBar->property("Demo").toBool() ? 0 : 2));
        break;
    case Benchmark::Latency:
        value = progressBar->property(metaEnum.valueToKey(Benchmark::Latency)).toFloat();
        progressBar->setFormat(value >= 1000000.0 ? locale.toString((int)value) : locale.toString(value, 'f', progressBar->property("Demo").toBool() ? 1 : 2));
        break;
    }

    if (!progressBar->property("Demo").toBool()) {
        if (comparisonField == Benchmark::Latency) {
            progressBar->setValue(value <= 0.0000000001 ? 0 : 100 - 16.666666666666 * log10(value));
        }
        else {
            progressBar->setValue(score <= 0.1 ? 0 : 16.666666666666 * log10(score * 10));
        }
    }
}

bool MainWindow::runCombinedRandomTest()
{
    if (m_benchmark->performanceProfile == Global::PerformanceProfile::Peak || m_benchmark->performanceProfile == Global::PerformanceProfile::RealWorld) {
        QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> set {
            { { Global::Test_2, Global::Read  }, { ui->readBar_2,  ui->readBar_3,  ui->readBar_4  } },
            { { Global::Test_2, Global::Write }, { ui->writeBar_2, ui->writeBar_3, ui->writeBar_4 } }
        };

        if (m_benchmark->isMixed()) {
            set << QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>
            { { Global::Test_2, Global::Mix   }, {  ui->mixBar_2,  ui->mixBar_3,   ui->mixBar_4   } };
        }

        m_benchmark->runBenchmark(set);

        return true;
    }

    return false;
}

void MainWindow::on_pushButton_Test_1_clicked()
{
    defineBenchmark([&]() {
        QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> set {
            { { Global::Test_1, Global::Read  }, { ui->readBar_1  } },
            { { Global::Test_1, Global::Write }, { ui->writeBar_1 } }
        };

        if (m_benchmark->isMixed()) {
            set << QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>
            { { Global::Test_1, Global::Mix   }, { ui->mixBar_1   } };
        }

        m_benchmark->runBenchmark(set);
    });
}

void MainWindow::on_pushButton_Test_2_clicked()
{
    defineBenchmark([&]() {
        if (runCombinedRandomTest()) return;

        QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> set {
            { { Global::Test_2, Global::Read  }, { ui->readBar_2  } },
            { { Global::Test_2, Global::Write }, { ui->writeBar_2 } }
        };

        if (m_benchmark->isMixed()) {
            set << QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>
            { { Global::Test_2, Global::Mix   }, { ui->mixBar_2   } };
        }

        m_benchmark->runBenchmark(set);
    });
}

void MainWindow::on_pushButton_Test_3_clicked()
{
    defineBenchmark([&]() {
        if (runCombinedRandomTest()) return;

        QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> set {
            { { Global::Test_3, Global::Read  }, { ui->readBar_3  } },
            { { Global::Test_3, Global::Write }, { ui->writeBar_3 } }
        };

        if (m_benchmark->isMixed()) {
            set << QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>
            { { Global::Test_3, Global::Mix   }, { ui->mixBar_3   } };
        }

        m_benchmark->runBenchmark(set);
    });
}

void MainWindow::on_pushButton_Test_4_clicked()
{
    defineBenchmark([&]() {
        if (runCombinedRandomTest()) return;

        QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> set {
            { { Global::Test_4, Global::Read  }, { ui->readBar_4  } },
            { { Global::Test_4, Global::Write }, { ui->writeBar_4 } }
        };

        if (m_benchmark->isMixed()) {
            set << QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>
            { { Global::Test_4, Global::Mix   }, { ui->mixBar_4   } };
        }

        m_benchmark->runBenchmark(set);
    });
}

void MainWindow::on_pushButton_All_clicked()
{
    defineBenchmark([&]() {
        QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> set;

        if (m_benchmark->performanceProfile == Global::PerformanceProfile::Default) {
            set << QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> {
                { { Global::Test_1, Global::Read  }, { ui->readBar_1  } },
                { { Global::Test_2, Global::Read  }, { ui->readBar_2  } },
                { { Global::Test_3, Global::Read  }, { ui->readBar_3  } },
                { { Global::Test_4, Global::Read  }, { ui->readBar_4  } },
                { { Global::Test_1, Global::Write }, { ui->writeBar_1 } },
                { { Global::Test_2, Global::Write }, { ui->writeBar_2 } },
                { { Global::Test_3, Global::Write }, { ui->writeBar_3 } },
                { { Global::Test_4, Global::Write }, { ui->writeBar_4 } }
            };

            if (m_benchmark->isMixed()) {
                set << QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> {
                { { Global::Test_1, Global::Mix   }, { ui->mixBar_1   } },
                { { Global::Test_2, Global::Mix   }, { ui->mixBar_2   } },
                { { Global::Test_3, Global::Mix   }, { ui->mixBar_3   } },
                { { Global::Test_4, Global::Mix   }, { ui->mixBar_4   } }
            };
            }
        }
        else if (m_benchmark->performanceProfile == Global::PerformanceProfile::Demo) {
            set << QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> {
                { { Global::Test_1, Global::Read  }, { ui->readBar_Demo  } },
                { { Global::Test_1, Global::Write }, { ui->writeBar_Demo } }
            };
        }
        else {
            set << QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> {
                { { Global::Test_1, Global::Read  }, { ui->readBar_1  } },
                { { Global::Test_2, Global::Read  }, { ui->readBar_2,  ui->readBar_3,  ui->readBar_4  } },
                { { Global::Test_1, Global::Write }, { ui->writeBar_1 } },
                { { Global::Test_2, Global::Write }, { ui->writeBar_2, ui->writeBar_3, ui->writeBar_4 } }
            };

            if (m_benchmark->isMixed()) {
                set << QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> {
                { { Global::Test_1, Global::Mix   }, { ui->mixBar_1   } },
                { { Global::Test_2, Global::Mix   }, { ui->mixBar_2,   ui->mixBar_3,   ui->mixBar_4   } }
            };
            }
        }

        m_benchmark->runBenchmark(set);
    });
}
