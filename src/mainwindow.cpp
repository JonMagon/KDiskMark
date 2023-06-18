#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QStorageInfo>
#include <QMetaEnum>
#include <QClipboard>
#include <QDate>
#include <QFileDialog>
#include <QTextStream>
#include <QAbstractItemView>
#include <QStyleFactory>
#include <QTimer>
#include <QStandardItemModel>

#include "math.h"
#include "about.h"
#include "settings.h"
#include "diskdriveinfo.h"
#include "storageitemdelegate.h"
#include "global.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_benchmark(new Benchmark)
{
    ui->setupUi(this);

#ifndef APPIMAGE_EDITION
    QMenuBar *bar = new QMenuBar(ui->menubar);

#ifdef SNAP_EDITION
    QAction *actionSnapSlot = new QAction(bar);
    actionSnapSlot->setIcon(style()->standardIcon(QStyle::SP_DriveHDIcon));

    connect(actionSnapSlot, &QAction::triggered, [this]() {
        QMessageBox::warning(this, "KDiskMark is limited", "External devices may not be available.\n"
                                                           "Use: sudo snap connect kdiskmark:removable-media");
    });

    bar->addAction(actionSnapSlot);
#endif

    QAction *actionLimited = new QAction(bar);
    actionLimited->setIcon(style()->standardIcon(QStyle::SP_MessageBoxWarning));

    connect(actionLimited, &QAction::triggered, [this]() {
        QMessageBox::warning(this, "KDiskMark is limited", "This edition of KDiskMark has limitations that cannot be fixed.\n"
                                                           "Clearing the cache and writing to protected directories will not be available.\n"
                                                           "Without clearing the cache the measured read performance may not be correct.\n"
                                                           "If necessary, use the native package for the distribution or AppImage.");
    });

    bar->addAction(actionLimited);

    bar->setStyleSheet("background-color: orange");
    ui->menubar->setCornerWidget(bar);
#endif

    QActionGroup *localesGroup = new QActionGroup(this);

    QVector<QLocale> locales = { QLocale::English, QLocale::Czech, QLocale::German,
                                 QLocale(QLocale::Spanish, QLocale::Mexico),
                                 QLocale::French, QLocale::Italian, QLocale::Hungarian, QLocale::Dutch,
                                 QLocale::Polish, QLocale(QLocale::Portuguese, QLocale::Brazil),
                                 QLocale::Slovak, QLocale::Swedish, QLocale::Turkish, QLocale::Russian,
                                 QLocale::Ukrainian, QLocale::Chinese, QLocale::Japanese, QLocale::Hindi };

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

    ui->refreshStoragesButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload).pixmap(QSize(16, 16)));

    ui->extraIcon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(QSize(16, 16)));
    ui->extraIcon->setToolTip(tr("The device is encrypted. Performance may drop."));
    ui->extraIcon->setVisible(false);

    statusBar()->hide();

    ui->loopsCount->findChild<QLineEdit*>()->setReadOnly(true);

    ui->comboBox_Storages->setItemDelegate(new StorageItemDelegate());

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

    ui->actionRead_Write_Mix->setProperty("mode", Global::BenchmarkMode::ReadWriteMix);
    ui->actionRead_Mix->setProperty("mode", Global::BenchmarkMode::ReadMix);
    ui->actionWrite_Mix->setProperty("mode", Global::BenchmarkMode::WriteMix);

    QActionGroup *modesGroup = new QActionGroup(this);
    ui->actionRead_Write_Mix->setActionGroup(modesGroup);
    ui->actionRead_Mix->setActionGroup(modesGroup);
    ui->actionWrite_Mix->setActionGroup(modesGroup);
    connect(modesGroup, SIGNAL(triggered(QAction*)), this, SLOT(modeSelected(QAction*)));

    ui->actionTestData_Random->setProperty("data", Global::BenchmarkTestData::Random);
    ui->actionTestData_Zeros->setProperty("data", Global::BenchmarkTestData::Zeros);

    QActionGroup *testDataGroup = new QActionGroup(this);
    ui->actionTestData_Random->setActionGroup(testDataGroup);
    ui->actionTestData_Zeros->setActionGroup(testDataGroup);
    connect(testDataGroup, SIGNAL(triggered(QAction*)), this, SLOT(testDataSelected(QAction*)));

    ui->actionPreset_Standard->setProperty("preset", Global::BenchmarkPreset::Standard);
    ui->actionPreset_NVMe_SSD->setProperty("preset", Global::BenchmarkPreset::NVMe_SSD);

    QActionGroup *presetsGroup = new QActionGroup(this);
    ui->actionPreset_Standard->setActionGroup(presetsGroup);
    ui->actionPreset_NVMe_SSD->setActionGroup(presetsGroup);
    connect(presetsGroup, SIGNAL(triggered(QAction*)), this, SLOT(presetSelected(QAction*)));

    ui->actionTheme_Use_Fusion->setProperty("theme", Global::Theme::UseFusion);
    ui->actionTheme_Stylesheet_Light->setProperty("theme", Global::Theme::StyleSheetLight);
    ui->actionTheme_Stylesheet_Dark->setProperty("theme", Global::Theme::StyleSheetDark);
    ui->actionTheme_Do_not_apply->setProperty("theme", Global::Theme::DoNotApply);

    QActionGroup *themeGroup = new QActionGroup(this);
    ui->actionTheme_Use_Fusion->setActionGroup(themeGroup);
    ui->actionTheme_Stylesheet_Light->setActionGroup(themeGroup);
    ui->actionTheme_Stylesheet_Dark->setActionGroup(themeGroup);
    ui->actionTheme_Do_not_apply->setActionGroup(themeGroup);
    connect(themeGroup, SIGNAL(triggered(QAction*)), this, SLOT(themeSelected(QAction*)));

    m_progressBars << ui->readBar_1 << ui->writeBar_1 << ui->mixBar_1
                   << ui->readBar_2 << ui->writeBar_2 << ui->mixBar_2
                   << ui->readBar_3 << ui->writeBar_3 << ui->mixBar_3
                   << ui->readBar_4 << ui->writeBar_4 << ui->mixBar_4
                   << ui->readBar_Demo << ui->writeBar_Demo;

    refreshProgressBars();

    // Load settings
    const AppSettings settings;

    for (QAction *action : { ui->actionDefault, ui->actionPeak_Performance, ui->actionReal_World_Performance, ui->actionDemo,
                             ui->actionDefault_Mix, ui->actionPeak_Performance_Mix, ui->actionReal_World_Performance_Mix }) {
        if (action->property("profile").toInt() == settings.getPerformanceProfile() && action->property("mixed").toBool() == settings.getMixedState()) {
            action->setChecked(true);
            profileSelected(action);
            break;
        }
    }

    int indexMixRatio = settings.getRandomReadPercentage() / 10 - 1;

    for (int i = 1; i <= 9; i++) {
        ui->comboBox_MixRatio->addItem(QStringLiteral("R%1%/W%2%").arg(i * 10).arg((10 - i) * 10));
    }

    ui->comboBox_ComparisonUnit->setCurrentIndex(settings.getComparisonUnit());
    ui->comboBox_MixRatio->setCurrentIndex(indexMixRatio);

    ui->actionTestData_Zeros->setChecked(settings.getBenchmarkTestData() == Global::BenchmarkTestData::Zeros);
    ui->actionRead_Mix->setChecked(settings.getBenchmarkMode() == Global::BenchmarkMode::ReadMix);
    ui->actionWrite_Mix->setChecked(settings.getBenchmarkMode() == Global::BenchmarkMode::WriteMix);

    ui->actionUse_O_DIRECT->setChecked(settings.getCacheBypassState());

#ifdef APPIMAGE_EDITION
    ui->actionFlush_Pagecache->setChecked(settings.getFlusingCacheState());
#else
    ui->actionFlush_Pagecache->setEnabled(false);
    ui->actionFlush_Pagecache->setChecked(false);
#endif

    ui->loopsCount->setValue(settings.getLoopsCount());

    ui->actionTheme_Stylesheet_Light->setChecked(settings.getTheme() == Global::Theme::StyleSheetLight);
    ui->actionTheme_Stylesheet_Dark->setChecked(settings.getTheme() ==Global::Theme::StyleSheetDark);
    ui->actionTheme_Do_not_apply->setChecked(settings.getTheme() == Global::Theme::DoNotApply);

    updateProgressBarsStyle();

    updatePresetsSelection();
    updateBenchmarkButtonsContent();

    updateFileSizeList();

    updateStoragesList();

    // Set callbacks
    connect(m_benchmark, &Benchmark::runningStateChanged, this, &MainWindow::benchmarkStateChanged);
    connect(m_benchmark, &Benchmark::benchmarkStatusUpdate, this, &MainWindow::benchmarkStatusUpdate);
    connect(m_benchmark, &Benchmark::resultReady, this, &MainWindow::handleResults);
    connect(m_benchmark, &Benchmark::failed, this, &MainWindow::benchmarkFailed);

    QTimer::singleShot(0, [&] {
        if (!m_benchmark->isFIODetected()) {
            QMessageBox::critical(this, "KDiskMark",
                                  QObject::tr("No FIO was found. Please install FIO before using KDiskMark."));
            qApp->quit();
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *)
{
    m_benchmark->setRunning(false);
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

        QMetaEnum metaEnum = QMetaEnum::fromType<Global::ComparisonUnit>();

        QLocale locale = QLocale();

        for (auto const& progressBar: m_progressBars) {
            if (!progressBar->property("Demo").toBool())
                progressBar->setToolTip(
                    Global::getToolTipTemplate().arg(
                        locale.toString(progressBar->property(metaEnum.valueToKey(Global::ComparisonUnit::MBPerSec)).toFloat(), 'f', 3),
                        locale.toString(progressBar->property(metaEnum.valueToKey(Global::ComparisonUnit::GBPerSec)).toFloat(), 'f', 3),
                        locale.toString(progressBar->property(metaEnum.valueToKey(Global::ComparisonUnit::IOPS)).toFloat(), 'f', 3),
                        locale.toString(progressBar->property(metaEnum.valueToKey(Global::ComparisonUnit::Latency)).toFloat(), 'f', 3)));
            updateProgressBar(progressBar);
        }

        for (int i = 0; i < ui->comboBox_Storages->count(); i++) {
            QVariant variant = ui->comboBox_Storages->itemData(i);
            if (variant.canConvert<Global::Storage>()) {
                Global::Storage storage = variant.value<Global::Storage>();

                storage.formatedSize = formatSize(storage.bytesOccupied, storage.bytesTotal);

                ui->comboBox_Storages->setItemText(i, QStringLiteral("%1 %2% (%3)").arg(storage.path)
                                                   .arg(storage.bytesOccupied * 100 / storage.bytesTotal)
                                                   .arg(storage.formatedSize));

                ui->comboBox_Storages->setItemData(i, QVariant::fromValue(storage));
            }
        }

        ui->comboBox_Storages->setItemText(0, tr("Add a directory"));
        resizeComboBoxItemsPopup(ui->comboBox_Storages);

        break;
    }
    default:
        QMainWindow::changeEvent(event);
    }
}

void MainWindow::on_refreshStoragesButton_clicked()
{
    updateStoragesList();
}

void MainWindow::updateStoragesList()
{
    QString temp;

    QVariant variant = ui->comboBox_Storages->currentData();
    if (variant.canConvert<Global::Storage>())
        temp = variant.value<Global::Storage>().path;

    QVector<Global::Storage> permanentStoragesInList;
    for (int i = 0; i < ui->comboBox_Storages->count(); i++) {
        QVariant variant = ui->comboBox_Storages->itemData(i);
        if (variant.canConvert<Global::Storage>()) {
            Global::Storage storage = variant.value<Global::Storage>();
            if (storage.permanentInList)
                permanentStoragesInList.append(storage);
        }
    }

    ui->comboBox_Storages->clear();

    QString homePath = QDir::homePath();
    QStorageInfo volume(homePath);

    Global::Storage storage {
        .path = homePath,
        .bytesTotal = volume.bytesTotal(),
        .bytesOccupied = volume.bytesTotal() - volume.bytesFree(),
        .formatedSize = formatSize(storage.bytesOccupied, storage.bytesTotal),
    };
    addItemToStoragesList(storage);

    foreach (const QStorageInfo &volume, QStorageInfo::mountedVolumes()) {
        if (volume.isValid() && volume.isReady() && !volume.isReadOnly()) {
            if (volume.device().indexOf("/dev") != -1) {
                QString rootPath = volume.rootPath();
#ifndef APPIMAGE_EDITION
                rootPath = rootPath.replace("/var/lib/snapd/hostfs", "");
                rootPath = rootPath.replace("/var/lib/snapd", "");
                if (rootPath.indexOf("/snap/kdiskmark") != -1)
                    rootPath = rootPath.mid(0, rootPath.indexOf("/snap/kdiskmark"));
                if (rootPath.isEmpty()) continue;
#endif
                Global::Storage storage {
                    .path = rootPath,
                    .bytesTotal = volume.bytesTotal(),
                    .bytesOccupied = volume.bytesTotal() - volume.bytesFree(),
                    .formatedSize = formatSize(storage.bytesOccupied, storage.bytesTotal),
                };
                addItemToStoragesList(storage);
            }
        }
    }

    for (Global::Storage storage : permanentStoragesInList) {
        storage.formatedSize = formatSize(storage.bytesOccupied, storage.bytesTotal);
        addItemToStoragesList(storage);
    }

    if (!temp.isEmpty()) {
        int foundIndex = ui->comboBox_Storages->findText(temp, Qt::MatchContains);
        if (foundIndex != -1) ui->comboBox_Storages->setCurrentIndex(foundIndex);
    }

    ui->comboBox_Storages->insertItem(0, tr("Add a directory"));
    ui->comboBox_Storages->setItemData(0, Qt::AlignCenter, Qt::TextAlignmentRole);
    ui->comboBox_Storages->setItemIcon(0, style()->standardIcon(QStyle::SP_FileDialogNewFolder));

    // Resize items popup
    resizeComboBoxItemsPopup(ui->comboBox_Storages);
}

void MainWindow::addItemToStoragesList(const Global::Storage &storage)
{
    if (ui->comboBox_Storages->findText(storage.path, Qt::MatchContains) != -1)
         return;

    ui->comboBox_Storages->addItem(
                QStringLiteral("%1 %2% (%3)").arg(storage.path)
                .arg(storage.bytesOccupied * 100 / storage.bytesTotal)
                .arg(storage.formatedSize),
                QVariant::fromValue(storage)
                );

    if (!QFileInfo(storage.path).isWritable()) {
        const QStandardItemModel* model =
                dynamic_cast<QStandardItemModel*>(ui->comboBox_Storages->model());

        QStandardItem* item = model->item(ui->comboBox_Storages->count() - 1);
        item->setEnabled(false);
    }
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

void MainWindow::on_actionUse_O_DIRECT_triggered(bool checked)
{
    AppSettings().setCacheBypassState(checked);
}

void MainWindow::on_actionFlush_Pagecache_triggered(bool checked)
{
    AppSettings().setFlushingCacheState(checked);
}

void MainWindow::updateBenchmarkButtonsContent()
{
    const AppSettings settings;

    Global::BenchmarkParams params;

    params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_1, settings.getPerformanceProfile());
    ui->pushButton_Test_1->setText(Global::getBenchmarkButtonText(params));

    switch (settings.getPerformanceProfile())
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

        params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_2, settings.getPerformanceProfile());
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

void MainWindow::updatePresetsSelection()
{
    const AppSettings settings;

    auto testFunc = [&] (Global::BenchmarkTest test, Global::PerformanceProfile profile, Global::BenchmarkPreset preset) {
        return settings.getBenchmarkParams(test, profile) == settings.defaultBenchmarkParams(test, profile, preset);
    };

    Global::BenchmarkPreset preset = Global::BenchmarkPreset::Standard;
    bool testStandardPreset =
            testFunc(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Default, preset) &&
            testFunc(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Default, preset) &&
            testFunc(Global::BenchmarkTest::Test_3, Global::PerformanceProfile::Default, preset) &&
            testFunc(Global::BenchmarkTest::Test_4, Global::PerformanceProfile::Default, preset) &&
            testFunc(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Peak, preset) &&
            testFunc(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Peak, preset) &&
            testFunc(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Demo, preset);

    ui->actionPreset_Standard->setChecked(testStandardPreset);

    if (!testStandardPreset) {
        Global::BenchmarkPreset preset = Global::BenchmarkPreset::NVMe_SSD;
        bool testNVMeSSDPreset =
                testFunc(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Default, preset) &&
                testFunc(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Default, preset) &&
                testFunc(Global::BenchmarkTest::Test_3, Global::PerformanceProfile::Default, preset) &&
                testFunc(Global::BenchmarkTest::Test_4, Global::PerformanceProfile::Default, preset) &&
                testFunc(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Peak, preset) &&
                testFunc(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Peak, preset) &&
                testFunc(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Demo, preset);

        ui->actionPreset_NVMe_SSD->setChecked(testNVMeSSDPreset);
    }
}

void MainWindow::refreshProgressBars()
{
    QMetaEnum metaEnum = QMetaEnum::fromType<Global::ComparisonUnit>();

    QLocale locale = QLocale();

    for (auto const& progressBar: m_progressBars) {
        progressBar->setProperty(metaEnum.valueToKey(Global::ComparisonUnit::MBPerSec), 0);
        progressBar->setProperty(metaEnum.valueToKey(Global::ComparisonUnit::GBPerSec), 0);
        progressBar->setProperty(metaEnum.valueToKey(Global::ComparisonUnit::IOPS), 0);
        progressBar->setProperty(metaEnum.valueToKey(Global::ComparisonUnit::Latency), 0);
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
    QLocale locale = QLocale();
    return QString("%1/%2 %3").arg(locale.toString(outputAvailable, 'f', 2))
            .arg(locale.toString(outputTotal, 'f', 2)).arg(units[i]);
}

void MainWindow::on_comboBox_ComparisonUnit_currentIndexChanged(int index)
{
    AppSettings().setComparisonUnit(Global::ComparisonUnit(index));
    updateLabels();

    for (auto const& progressBar: m_progressBars) {
        updateProgressBar(progressBar);
    }
}

void MainWindow::updateLabels()
{
    ui->label_Read->setText(Global::getComparisonLabelTemplate()
                            .arg(tr("Read"), ui->comboBox_ComparisonUnit->currentText()));

    ui->label_Write->setText(Global::getComparisonLabelTemplate()
                             .arg(tr("Write"), ui->comboBox_ComparisonUnit->currentText()));

    ui->label_Mix->setText(Global::getComparisonLabelTemplate()
                             .arg(tr("Mix"), ui->comboBox_ComparisonUnit->currentText()));

    ui->label_Unit_Read_Demo->setText(ui->comboBox_ComparisonUnit->currentText());
    ui->label_Unit_Write_Demo->setText(ui->comboBox_ComparisonUnit->currentText());
}

QString MainWindow::combineOutputTestResult(const QProgressBar *progressBar, const Global::BenchmarkParams &params)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<Global::ComparisonUnit>();

    return QString("%1 %2 %3 (Q=%4, T=%5): %6 MB/s [ %7 IOPS] < %8 us>")
           .arg(params.Pattern == Global::BenchmarkIOPattern::SEQ ? "Sequential" : "Random")
           .arg(QString::number(params.BlockSize >= 1024 ? params.BlockSize / 1024 : params.BlockSize).rightJustified(3, ' '))
           .arg(params.BlockSize >= 1024 ? "MiB" : "KiB")
           .arg(QString::number(params.Queues).rightJustified(3, ' '))
           .arg(QString::number(params.Threads).rightJustified(2, ' '))
           .arg(QString::number(
                    progressBar->property(metaEnum.valueToKey(Global::ComparisonUnit::MBPerSec)).toFloat(), 'f', 3)
                .rightJustified(9, ' '))
           .arg(QString::number(
                    progressBar->property(metaEnum.valueToKey(Global::ComparisonUnit::IOPS)).toFloat(), 'f', 1)
                .rightJustified(8, ' '))
           .arg(QString::number(
                    progressBar->property(metaEnum.valueToKey(Global::ComparisonUnit::Latency)).toFloat(), 'f', 2)
                .rightJustified(8, ' '))
           .rightJustified(Global::getOutputColumnsCount(), ' ');
}

QString MainWindow::getTextBenchmarkResult()
{
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
           << "[Read]";
    if (settings.getPerformanceProfile() == Global::PerformanceProfile::Demo) {
        output << combineOutputTestResult(ui->readBar_Demo, settings.getBenchmarkParams(Global::BenchmarkTest::Test_1, settings.getPerformanceProfile()));
    }
    else {
        output << combineOutputTestResult(ui->readBar_1, settings.getBenchmarkParams(Global::BenchmarkTest::Test_1, settings.getPerformanceProfile()));
        output << combineOutputTestResult(ui->readBar_2, settings.getBenchmarkParams(Global::BenchmarkTest::Test_2, settings.getPerformanceProfile()));
        if (settings.getPerformanceProfile() == Global::PerformanceProfile::Default) {
            output << combineOutputTestResult(ui->readBar_3, settings.getBenchmarkParams(Global::BenchmarkTest::Test_3, settings.getPerformanceProfile()));
            output << combineOutputTestResult(ui->readBar_4, settings.getBenchmarkParams(Global::BenchmarkTest::Test_4, settings.getPerformanceProfile()));
        }
    }

    output << QString()
           << "[Write]";
    if (settings.getPerformanceProfile() == Global::PerformanceProfile::Demo) {
        output << combineOutputTestResult(ui->writeBar_Demo, settings.getBenchmarkParams(Global::BenchmarkTest::Test_1, settings.getPerformanceProfile()));
    }
    else {
        output << combineOutputTestResult(ui->writeBar_1, settings.getBenchmarkParams(Global::BenchmarkTest::Test_1, settings.getPerformanceProfile()));
        output << combineOutputTestResult(ui->writeBar_2, settings.getBenchmarkParams(Global::BenchmarkTest::Test_2, settings.getPerformanceProfile()));
        if (settings.getPerformanceProfile() == Global::PerformanceProfile::Default) {
            output << combineOutputTestResult(ui->writeBar_3, settings.getBenchmarkParams(Global::BenchmarkTest::Test_3, settings.getPerformanceProfile()));
            output << combineOutputTestResult(ui->writeBar_4, settings.getBenchmarkParams(Global::BenchmarkTest::Test_4, settings.getPerformanceProfile()));
        }
    }

    if (settings.getMixedState() && settings.getPerformanceProfile() != Global::PerformanceProfile::Demo) {
         output << QString()
                << QString("[Mix] Read %1%/Write %2%")
                   .arg(settings.getRandomReadPercentage())
                   .arg(100 - settings.getRandomReadPercentage())
                << combineOutputTestResult(ui->mixBar_1, settings.getBenchmarkParams(Global::BenchmarkTest::Test_1, settings.getPerformanceProfile()))
                << combineOutputTestResult(ui->mixBar_2, settings.getBenchmarkParams(Global::BenchmarkTest::Test_2, settings.getPerformanceProfile()));
             if (settings.getPerformanceProfile() == Global::PerformanceProfile::Default) {
                 output << combineOutputTestResult(ui->mixBar_3, settings.getBenchmarkParams(Global::BenchmarkTest::Test_3, settings.getPerformanceProfile()));
                 output << combineOutputTestResult(ui->mixBar_4, settings.getBenchmarkParams(Global::BenchmarkTest::Test_4, settings.getPerformanceProfile()));
             }
    }

    QString profiles[] = { "Default", "Peak Performance", "Real World Performance", "Demo" };

    output << QString()
           << QString("Profile: %1%2")
              .arg(profiles[(int)settings.getPerformanceProfile()]).arg(settings.getMixedState() ? " [+Mix]" : QString())
           << QString("   Test: %1")
              .arg("%1 %2 (x%3) [Measure: %4 %5 / Interval: %6 %7]")
              .arg(settings.getFileSize() >= 1024 ? settings.getFileSize() / 1024 : settings.getFileSize())
              .arg(settings.getFileSize() >= 1024 ? "GiB" : "MiB")
              .arg(settings.getLoopsCount())
              .arg(settings.getMeasuringTime() >= 60 ? settings.getMeasuringTime() / 60 : settings.getMeasuringTime())
              .arg(settings.getMeasuringTime() >= 60 ? "min" : "sec")
              .arg(settings.getIntervalTime() >= 60 ? settings.getIntervalTime() / 60 : settings.getIntervalTime())
              .arg(settings.getIntervalTime() >= 60 ? "min" : "sec")
           << QString("   Date: %1 %2")
              .arg(QDate::currentDate().toString("yyyy-MM-dd"))
              .arg(QTime::currentTime().toString("hh:mm:ss"))
           << QString("     OS: %1 %2 [%3 %4]").arg(QSysInfo::productType()).arg(QSysInfo::productVersion())
              .arg(QSysInfo::kernelType()).arg(QSysInfo::kernelVersion());

    return output.join("\n");
}

void MainWindow::on_actionCopy_triggered()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(getTextBenchmarkResult());
}

void MainWindow::on_actionSave_triggered()
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
    // It is expected that at index 0 without item data there will be a field to add a directory
    if (index == 0 && ui->comboBox_Storages->itemData(index).isNull()) {
        QString dir = QFileDialog::getExistingDirectory(this, QString(), QDir::homePath(),
                                                        QFileDialog::ShowDirsOnly |
                                                        QFileDialog::DontResolveSymlinks
                                                #ifdef SNAP_EDITION
                                                        | QFileDialog::DontUseNativeDialog
                                                #endif
                                                        );
        if (!dir.isNull()) {
            int foundIndex = ui->comboBox_Storages->findText(dir, Qt::MatchContains);

            if (foundIndex == -1) {
                QStorageInfo volume(dir);

                Global::Storage storage {
                    .path = dir,
                    .bytesTotal = volume.bytesTotal(),
                    .bytesOccupied = volume.bytesTotal() - volume.bytesFree(),
                    .formatedSize = formatSize(storage.bytesOccupied, storage.bytesTotal),
                    .permanentInList = true
                };

                addItemToStoragesList(storage);
                resizeComboBoxItemsPopup(ui->comboBox_Storages);

                ui->comboBox_Storages->setCurrentIndex(ui->comboBox_Storages->count() - 1);
            }
            else {
                ui->comboBox_Storages->setCurrentIndex(foundIndex);
            }

            return;
        }

        ui->comboBox_Storages->setCurrentIndex(1);
    }
    else {
        QVariant variant = ui->comboBox_Storages->itemData(index);
        if (variant.canConvert<Global::Storage>()) {
            Global::Storage volumeInfo = variant.value<Global::Storage>();
            m_benchmark->setDir(volumeInfo.path);
            ui->deviceModel->setText(DiskDriveInfo::Instance().getModelName(QStorageInfo(volumeInfo.path).device()));
            ui->extraIcon->setVisible(DiskDriveInfo::Instance().isEncrypted(QStorageInfo(volumeInfo.path).device()));
        }
    }
}

void MainWindow::localeSelected(QAction* act)
{
    AppSettings settings;

    if (!act->data().canConvert<QLocale>()) return;

    settings.setLocale(act->data().toLocale());
}

void MainWindow::profileSelected(QAction* act)
{
    AppSettings settings;

    bool isMixed = act->property("mixed").toBool();

    settings.setMixedState(isMixed);

    ui->mixWidget->setVisible(isMixed);
    ui->comboBox_MixRatio->setVisible(isMixed);

    switch (act->property("profile").toInt())
    {
    case Global::PerformanceProfile::Default:
        m_windowTitle = "KDiskMark";
        ui->comboBox_ComparisonUnit->setVisible(true);
        break;
    case Global::PerformanceProfile::Peak:
        m_windowTitle = "KDiskMark <PEAK>";
        ui->comboBox_ComparisonUnit->setCurrentIndex(0);
        ui->comboBox_ComparisonUnit->setVisible(false);
        break;
    case Global::PerformanceProfile::RealWorld:
        m_windowTitle = "KDiskMark <REAL>";
        ui->comboBox_ComparisonUnit->setCurrentIndex(0);
        ui->comboBox_ComparisonUnit->setVisible(false);
        break;
    case Global::PerformanceProfile::Demo:
        m_windowTitle = "KDiskMark <DEMO>";
        ui->comboBox_ComparisonUnit->setVisible(true);
        break;
    }

    setWindowTitle(m_windowTitle);

    settings.setPerformanceProfile((Global::PerformanceProfile)act->property("profile").toInt());

    ui->stackedWidget->setCurrentIndex(settings.getPerformanceProfile() == Global::PerformanceProfile::Demo ? 1 : 0);

    int right = (isMixed ? ui->mixWidget->geometry().right() : ui->writeWidget->geometry().right()) + ui->stackedWidget->geometry().x();

    ui->targetLayoutWidget->resize(right - ui->targetLayoutWidget->geometry().left(),
                                   ui->targetLayoutWidget->geometry().height());
    ui->commentLayoutWidget->resize(right - ui->commentLayoutWidget->geometry().left(),
                                    ui->commentLayoutWidget->geometry().height());

    setFixedWidth(ui->commentLayoutWidget->geometry().width() + 2 * ui->commentLayoutWidget->geometry().left());

    refreshProgressBars();
    updateBenchmarkButtonsContent();
}

void MainWindow::modeSelected(QAction* act)
{
    AppSettings().setBenchmarkMode((Global::BenchmarkMode)act->property("mode").toInt());
}

void MainWindow::testDataSelected(QAction* act)
{
    AppSettings().setBenchmarkTestData((Global::BenchmarkTestData)act->property("data").toInt());
}

void MainWindow::presetSelected(QAction* act)
{
    AppSettings settings;

    Global::BenchmarkPreset preset = (Global::BenchmarkPreset)act->property("preset").toInt();

    auto updateFunc = [&] (Global::BenchmarkTest test, Global::PerformanceProfile profile) {
        settings.setBenchmarkParams(test, profile, settings.defaultBenchmarkParams(test, profile, preset));
    };

    updateFunc(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Default);
    updateFunc(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Default);
    updateFunc(Global::BenchmarkTest::Test_3, Global::PerformanceProfile::Default);
    updateFunc(Global::BenchmarkTest::Test_4, Global::PerformanceProfile::Default);

    updateFunc(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Peak);
    updateFunc(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Peak);

    updateFunc(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Demo);

    updateBenchmarkButtonsContent();
}


void MainWindow::updateProgressBarsStyle()
{
    QStyle *progressBarStyleFusion = QStyleFactory::create(QStringLiteral("Fusion"));
    QStyle *progressBarStyleDefault = QStyleFactory::create(QStringLiteral());

    Global::Theme theme = AppSettings().getTheme();

    for (auto const& progressBar: m_progressBars) {
        switch (theme) {
        case Global::Theme::UseFusion:
            progressBar->setStyle(progressBarStyleFusion);
            progressBar->setStyleSheet(QStringLiteral());
            break;
        case Global::Theme::StyleSheetLight:
            progressBar->setStyle(progressBarStyleDefault);
            progressBar->setStyleSheet(QStringLiteral("QProgressBar{color:black;text-align:center}QProgressBar::chunk{background:rgba(0,0,0,0)}"));
            break;
        case Global::Theme::StyleSheetDark:
            progressBar->setStyle(progressBarStyleDefault);
            progressBar->setStyleSheet(QStringLiteral("QProgressBar{color:white;text-align:center}QProgressBar::chunk{background:rgba(0,0,0,0)}"));
            break;
        case Global::Theme::DoNotApply:
            progressBar->setStyle(progressBarStyleDefault);
            progressBar->setStyleSheet(QStringLiteral());
            break;
        }
    }
}

void MainWindow::themeSelected(QAction* act)
{
    AppSettings().setTheme((Global::Theme)act->property("theme").toInt());

    updateProgressBarsStyle();
}


void MainWindow::benchmarkStateChanged(bool state)
{
    if (state) {
        ui->menubar->setEnabled(false);
#ifndef APPIMAGE_EDITION
        ui->menubar->cornerWidget()->setStyleSheet(QStringLiteral());
#endif
        ui->loopsCount->setEnabled(false);
        ui->comboBox_fileSize->setEnabled(false);
        ui->comboBox_Storages->setEnabled(false);
        ui->refreshStoragesButton->setEnabled(false);
        ui->comboBox_ComparisonUnit->setEnabled(false);
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
#ifndef APPIMAGE_EDITION
        ui->menubar->cornerWidget()->setStyleSheet("background-color: orange");
#endif
        ui->loopsCount->setEnabled(true);
        ui->comboBox_fileSize->setEnabled(true);
        ui->comboBox_Storages->setEnabled(true);
        ui->refreshStoragesButton->setEnabled(true);
        ui->comboBox_ComparisonUnit->setEnabled(true);
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

void MainWindow::on_actionAbout_triggered()
{
    About about(m_benchmark->getFIOVersion());
    about.setFixedSize(about.size());
    about.exec();
}

void MainWindow::on_actionQueues_Threads_triggered()
{
    Settings settings;
    settings.setFixedSize(settings.size());
    settings.exec();

    updatePresetsSelection();
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
        else {
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
    QMetaEnum metaEnum = QMetaEnum::fromType<Global::ComparisonUnit>();

    progressBar->setProperty(metaEnum.valueToKey(Global::ComparisonUnit::MBPerSec), result.Bandwidth);
    progressBar->setProperty(metaEnum.valueToKey(Global::ComparisonUnit::GBPerSec), result.Bandwidth / 1000);
    progressBar->setProperty(metaEnum.valueToKey(Global::ComparisonUnit::IOPS), result.IOPS);
    progressBar->setProperty(metaEnum.valueToKey(Global::ComparisonUnit::Latency), result.Latency);

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
    const AppSettings settings;

    QMetaEnum metaEnum = QMetaEnum::fromType<Global::ComparisonUnit>();

    float score = progressBar->property(metaEnum.valueToKey(Global::ComparisonUnit::MBPerSec)).toFloat();

    float value;

    Global::ComparisonUnit comparisonUnit = Global::ComparisonUnit::MBPerSec;

    switch (settings.getPerformanceProfile()) {
    case Global::PerformanceProfile::Peak:
    case Global::PerformanceProfile::RealWorld:
        if (progressBar == ui->readBar_3 || progressBar == ui->writeBar_3 || progressBar == ui->mixBar_3) {
            comparisonUnit = Global::ComparisonUnit::IOPS;
        }
        else if (progressBar == ui->readBar_4 || progressBar == ui->writeBar_4 || progressBar == ui->mixBar_4) {
            comparisonUnit = Global::ComparisonUnit::Latency;
        }
        break;
    default:
        comparisonUnit = settings.getComparisonUnit();
        break;
    }

    QLocale locale = QLocale();

    switch (comparisonUnit) {
    case Global::ComparisonUnit::MBPerSec:
        progressBar->setFormat(score >= 1000000.0 ? locale.toString((int)score) : locale.toString(score, 'f', progressBar->property("Demo").toBool() ? 1 : 2));
        break;
    case Global::ComparisonUnit::GBPerSec:
        value = progressBar->property(metaEnum.valueToKey(Global::ComparisonUnit::GBPerSec)).toFloat();
        progressBar->setFormat(locale.toString(value, 'f', progressBar->property("Demo").toBool() ? 1 : 3));
        break;
    case Global::ComparisonUnit::IOPS:
        value = progressBar->property(metaEnum.valueToKey(Global::ComparisonUnit::IOPS)).toFloat();
        progressBar->setFormat(value >= 1000000.0 ? locale.toString((int)value) : locale.toString(value, 'f', progressBar->property("Demo").toBool() ? 0 : 2));
        break;
    case Global::ComparisonUnit::Latency:
        value = progressBar->property(metaEnum.valueToKey(Global::ComparisonUnit::Latency)).toFloat();
        progressBar->setFormat(value >= 1000000.0 ? locale.toString((int)value) : locale.toString(value, 'f', progressBar->property("Demo").toBool() ? 1 : 2));
        break;
    }

    if (!progressBar->property("Demo").toBool()) {
        if (comparisonUnit == Global::ComparisonUnit::Latency) {
            progressBar->setValue(value <= 0.0000000001 ? 0 : 100 - 16.666666666666 * log10(value));
        }
        else {
            progressBar->setValue(score <= 0.1 ? 0 : 16.666666666666 * log10(score * 10));
        }
    }
}

bool MainWindow::runCombinedRandomTest()
{
    const AppSettings settings;

    if (settings.getPerformanceProfile() == Global::PerformanceProfile::Peak || settings.getPerformanceProfile() == Global::PerformanceProfile::RealWorld) {
        QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> set {
            { { Global::Test_2, Global::Read  }, { ui->readBar_2,  ui->readBar_3,  ui->readBar_4  } },
            { { Global::Test_2, Global::Write }, { ui->writeBar_2, ui->writeBar_3, ui->writeBar_4 } }
        };

        if (settings.getMixedState()) {
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

        if (AppSettings().getMixedState()) {
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

        if (AppSettings().getMixedState()) {
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

        if (AppSettings().getMixedState()) {
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

        if (AppSettings().getMixedState()) {
            set << QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>
            { { Global::Test_4, Global::Mix   }, { ui->mixBar_4   } };
        }

        m_benchmark->runBenchmark(set);
    });
}

void MainWindow::on_pushButton_All_clicked()
{
    defineBenchmark([&]() {
        const AppSettings settings;

        QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> set;

        if (settings.getPerformanceProfile() == Global::PerformanceProfile::Default) {
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

            if (settings.getMixedState()) {
                set << QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> {
                { { Global::Test_1, Global::Mix   }, { ui->mixBar_1   } },
                { { Global::Test_2, Global::Mix   }, { ui->mixBar_2   } },
                { { Global::Test_3, Global::Mix   }, { ui->mixBar_3   } },
                { { Global::Test_4, Global::Mix   }, { ui->mixBar_4   } }
            };
            }
        }
        else if (settings.getPerformanceProfile() == Global::PerformanceProfile::Demo) {
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

            if (settings.getMixedState()) {
                set << QList<QPair<QPair<Global::BenchmarkTest, Global::BenchmarkIOReadWrite>, QVector<QProgressBar*>>> {
                { { Global::Test_1, Global::Mix   }, { ui->mixBar_1   } },
                { { Global::Test_2, Global::Mix   }, { ui->mixBar_2,   ui->mixBar_3,   ui->mixBar_4   } }
            };
            }
        }

        m_benchmark->runBenchmark(set);
    });
}
