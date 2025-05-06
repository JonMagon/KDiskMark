#include "settings.h"
#include "ui_settings.h"

#include <QMetaEnum>

#include "appsettings.h"
#include "global.h"

Settings::Settings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);

    ui->buttonBox->addButton(QStringLiteral("NVMe SSD"), QDialogButtonBox::ActionRole);

    for (int val : { 0, 1, 3, 5, 10, 30, 60, 180, 300, 600 }) {
        ui->IntervalTime->addItem(val < 60 ? QStringLiteral("%1 %2").arg(val).arg(tr("sec"))
                                           : QStringLiteral("%1 %2").arg(val / 60).arg(tr("min")), val);
    }

    for (int val : { 5, 10, 20, 30, 60 }) {
        ui->MeasuringTime->addItem(val < 60 ? QStringLiteral("%1 %2").arg(val).arg(tr("sec"))
                                            : QStringLiteral("%1 %2").arg(val / 60).arg(tr("min")), val);
    }

    for (const Global::BenchmarkIOPattern &pattern : { Global::BenchmarkIOPattern::SEQ, Global::BenchmarkIOPattern::RND }) {
        QString patternName = QMetaEnum::fromType<Global::BenchmarkIOPattern>().valueToKey(pattern);

        ui->DefaultProfile_Test_1_Pattern->addItem(patternName, pattern);
        ui->DefaultProfile_Test_2_Pattern->addItem(patternName, pattern);
        ui->DefaultProfile_Test_3_Pattern->addItem(patternName, pattern);
        ui->DefaultProfile_Test_4_Pattern->addItem(patternName, pattern);

        ui->PeakPerformanceProfile_Test_1_Pattern->addItem(patternName, pattern);
        ui->PeakPerformanceProfile_Test_2_Pattern->addItem(patternName, pattern);

        ui->DemoProfile_Test_1_Pattern->addItem(patternName, pattern);
    }

    QString i_str, j_str;

    for (int i = 1, j = 1; i <= 64; i++, (j <= 8192 ? j *= 2 : j)) {
        i_str = QString::number(i);
        j_str = QString::number(j);

        if (j <= 512) {
            ui->DefaultProfile_Test_1_Queues->addItem(j_str, j);
            ui->DefaultProfile_Test_2_Queues->addItem(j_str, j);
            ui->DefaultProfile_Test_3_Queues->addItem(j_str, j);
            ui->DefaultProfile_Test_4_Queues->addItem(j_str, j);

            ui->PeakPerformanceProfile_Test_1_Queues->addItem(j_str, j);
            ui->PeakPerformanceProfile_Test_2_Queues->addItem(j_str, j);

            ui->DemoProfile_Test_1_Queues->addItem(j_str, j);

            if (j >= 4) {
                ui->DefaultProfile_Test_1_BlockSize->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("KiB")), j);
                ui->DefaultProfile_Test_2_BlockSize->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("KiB")), j);
                ui->DefaultProfile_Test_3_BlockSize->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("KiB")), j);
                ui->DefaultProfile_Test_4_BlockSize->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("KiB")), j);

                ui->PeakPerformanceProfile_Test_1_BlockSize->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("KiB")), j);
                ui->PeakPerformanceProfile_Test_2_BlockSize->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("KiB")), j);

                ui->DemoProfile_Test_1_BlockSize->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("KiB")), j);
            }
        }
        else if (j <= 8192) {
            ui->DefaultProfile_Test_1_BlockSize->addItem(QStringLiteral("%1 %2").arg(j / 1024).arg(tr("MiB")), j);
            ui->DefaultProfile_Test_2_BlockSize->addItem(QStringLiteral("%1 %2").arg(j / 1024).arg(tr("MiB")), j);
            ui->DefaultProfile_Test_3_BlockSize->addItem(QStringLiteral("%1 %2").arg(j / 1024).arg(tr("MiB")), j);
            ui->DefaultProfile_Test_4_BlockSize->addItem(QStringLiteral("%1 %2").arg(j / 1024).arg(tr("MiB")), j);

            ui->PeakPerformanceProfile_Test_1_BlockSize->addItem(QStringLiteral("%1 %2").arg(j / 1024).arg(tr("MiB")), j);
            ui->PeakPerformanceProfile_Test_2_BlockSize->addItem(QStringLiteral("%1 %2").arg(j / 1024).arg(tr("MiB")), j);

            ui->DemoProfile_Test_1_BlockSize->addItem(QStringLiteral("%1 %2").arg(j / 1024).arg(tr("MiB")), j);
        }

        ui->DefaultProfile_Test_1_Threads->addItem(i_str, i);
        ui->DefaultProfile_Test_2_Threads->addItem(i_str, i);
        ui->DefaultProfile_Test_3_Threads->addItem(i_str, i);
        ui->DefaultProfile_Test_4_Threads->addItem(i_str, i);

        ui->PeakPerformanceProfile_Test_1_Threads->addItem(i_str, i);
        ui->PeakPerformanceProfile_Test_2_Threads->addItem(i_str, i);

        ui->DemoProfile_Test_1_Threads->addItem(i_str, i);
    }

    const AppSettings settings;

    Global::BenchmarkParams params;

    params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Default);
    findDataAndSet(ui->DefaultProfile_Test_1_Pattern, params.Pattern);
    findDataAndSet(ui->DefaultProfile_Test_1_BlockSize, params.BlockSize);
    findDataAndSet(ui->DefaultProfile_Test_1_Queues, params.Queues);
    findDataAndSet(ui->DefaultProfile_Test_1_Threads, params.Threads);

    params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Default);
    findDataAndSet(ui->DefaultProfile_Test_2_Pattern, params.Pattern);
    findDataAndSet(ui->DefaultProfile_Test_2_BlockSize, params.BlockSize);
    findDataAndSet(ui->DefaultProfile_Test_2_Queues, params.Queues);
    findDataAndSet(ui->DefaultProfile_Test_2_Threads, params.Threads);

    params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_3, Global::PerformanceProfile::Default);
    findDataAndSet(ui->DefaultProfile_Test_3_Pattern, params.Pattern);
    findDataAndSet(ui->DefaultProfile_Test_3_BlockSize, params.BlockSize);
    findDataAndSet(ui->DefaultProfile_Test_3_Queues, params.Queues);
    findDataAndSet(ui->DefaultProfile_Test_3_Threads, params.Threads);

    params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_4, Global::PerformanceProfile::Default);
    findDataAndSet(ui->DefaultProfile_Test_4_Pattern, params.Pattern);
    findDataAndSet(ui->DefaultProfile_Test_4_BlockSize, params.BlockSize);
    findDataAndSet(ui->DefaultProfile_Test_4_Queues, params.Queues);
    findDataAndSet(ui->DefaultProfile_Test_4_Threads, params.Threads);


    params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Peak);
    findDataAndSet(ui->PeakPerformanceProfile_Test_1_Pattern, params.Pattern);
    findDataAndSet(ui->PeakPerformanceProfile_Test_1_BlockSize, params.BlockSize);
    findDataAndSet(ui->PeakPerformanceProfile_Test_1_Queues, params.Queues);
    findDataAndSet(ui->PeakPerformanceProfile_Test_1_Threads, params.Threads);

    params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Peak);
    findDataAndSet(ui->PeakPerformanceProfile_Test_2_Pattern, params.Pattern);
    findDataAndSet(ui->PeakPerformanceProfile_Test_2_BlockSize, params.BlockSize);
    findDataAndSet(ui->PeakPerformanceProfile_Test_2_Queues, params.Queues);
    findDataAndSet(ui->PeakPerformanceProfile_Test_2_Threads, params.Threads);


    params = settings.getBenchmarkParams(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Demo);
    findDataAndSet(ui->DemoProfile_Test_1_Pattern, params.Pattern);
    findDataAndSet(ui->DemoProfile_Test_1_BlockSize, params.BlockSize);
    findDataAndSet(ui->DemoProfile_Test_1_Queues, params.Queues);
    findDataAndSet(ui->DemoProfile_Test_1_Threads, params.Threads);


    findDataAndSet(ui->MeasuringTime, settings.getMeasuringTime());
    findDataAndSet(ui->IntervalTime, settings.getIntervalTime());
}

Settings::~Settings()
{
    delete ui;
}

void Settings::findDataAndSet(QComboBox *comboBox, int data)
{
    comboBox->setCurrentIndex(comboBox->findData(data));
}

void Settings::on_buttonBox_clicked(QAbstractButton *button)
{
    AppSettings settings;

    if (ui->buttonBox->standardButton(button) == QDialogButtonBox::Ok) {
        settings.setBenchmarkParams(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Default, {
                                        (Global::BenchmarkIOPattern)ui->DefaultProfile_Test_1_Pattern->currentData().toInt(),
                                        ui->DefaultProfile_Test_1_BlockSize->currentData().toInt(),
                                        ui->DefaultProfile_Test_1_Queues->currentData().toInt(),
                                        ui->DefaultProfile_Test_1_Threads->currentData().toInt()
                                    });

        settings.setBenchmarkParams(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Default, {
                                        (Global::BenchmarkIOPattern)ui->DefaultProfile_Test_2_Pattern->currentData().toInt(),
                                        ui->DefaultProfile_Test_2_BlockSize->currentData().toInt(),
                                        ui->DefaultProfile_Test_2_Queues->currentData().toInt(),
                                        ui->DefaultProfile_Test_2_Threads->currentData().toInt()
                                    });

        settings.setBenchmarkParams(Global::BenchmarkTest::Test_3, Global::PerformanceProfile::Default, {
                                        (Global::BenchmarkIOPattern)ui->DefaultProfile_Test_3_Pattern->currentData().toInt(),
                                        ui->DefaultProfile_Test_3_BlockSize->currentData().toInt(),
                                        ui->DefaultProfile_Test_3_Queues->currentData().toInt(),
                                        ui->DefaultProfile_Test_3_Threads->currentData().toInt()
                                    });

        settings.setBenchmarkParams(Global::BenchmarkTest::Test_4, Global::PerformanceProfile::Default, {
                                        (Global::BenchmarkIOPattern)ui->DefaultProfile_Test_4_Pattern->currentData().toInt(),
                                        ui->DefaultProfile_Test_4_BlockSize->currentData().toInt(),
                                        ui->DefaultProfile_Test_4_Queues->currentData().toInt(),
                                        ui->DefaultProfile_Test_4_Threads->currentData().toInt()
                                    });


        settings.setBenchmarkParams(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Peak, {
                                        (Global::BenchmarkIOPattern)ui->PeakPerformanceProfile_Test_1_Pattern->currentData().toInt(),
                                        ui->PeakPerformanceProfile_Test_1_BlockSize->currentData().toInt(),
                                        ui->PeakPerformanceProfile_Test_1_Queues->currentData().toInt(),
                                        ui->PeakPerformanceProfile_Test_1_Threads->currentData().toInt()
                                    });

        settings.setBenchmarkParams(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Peak, {
                                        (Global::BenchmarkIOPattern)ui->PeakPerformanceProfile_Test_2_Pattern->currentData().toInt(),
                                        ui->PeakPerformanceProfile_Test_2_BlockSize->currentData().toInt(),
                                        ui->PeakPerformanceProfile_Test_2_Queues->currentData().toInt(),
                                        ui->PeakPerformanceProfile_Test_2_Threads->currentData().toInt()
                                    });


        settings.setBenchmarkParams(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Demo, {
                                        (Global::BenchmarkIOPattern)ui->DemoProfile_Test_1_Pattern->currentData().toInt(),
                                        ui->DemoProfile_Test_1_BlockSize->currentData().toInt(),
                                        ui->DemoProfile_Test_1_Queues->currentData().toInt(),
                                        ui->DemoProfile_Test_1_Threads->currentData().toInt()
                                    });

        settings.setMeasuringTime(ui->MeasuringTime->currentData().toInt());
        settings.setIntervalTime(ui->IntervalTime->currentData().toInt());

        close();
    }
    else if (ui->buttonBox->standardButton(button) == QDialogButtonBox::RestoreDefaults) {
        Global::BenchmarkParams params;

        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Default, Global::BenchmarkPreset::Standard);
        findDataAndSet(ui->DefaultProfile_Test_1_Pattern, params.Pattern);
        findDataAndSet(ui->DefaultProfile_Test_1_BlockSize, params.BlockSize);
        findDataAndSet(ui->DefaultProfile_Test_1_Queues, params.Queues);
        findDataAndSet(ui->DefaultProfile_Test_1_Threads, params.Threads);

        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Default, Global::BenchmarkPreset::Standard);
        findDataAndSet(ui->DefaultProfile_Test_2_Pattern, params.Pattern);
        findDataAndSet(ui->DefaultProfile_Test_2_BlockSize, params.BlockSize);
        findDataAndSet(ui->DefaultProfile_Test_2_Queues, params.Queues);
        findDataAndSet(ui->DefaultProfile_Test_2_Threads, params.Threads);

        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_3, Global::PerformanceProfile::Default, Global::BenchmarkPreset::Standard);
        findDataAndSet(ui->DefaultProfile_Test_3_Pattern, params.Pattern);
        findDataAndSet(ui->DefaultProfile_Test_3_BlockSize, params.BlockSize);
        findDataAndSet(ui->DefaultProfile_Test_3_Queues, params.Queues);
        findDataAndSet(ui->DefaultProfile_Test_3_Threads, params.Threads);

        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_4, Global::PerformanceProfile::Default, Global::BenchmarkPreset::Standard);
        findDataAndSet(ui->DefaultProfile_Test_4_Pattern, params.Pattern);
        findDataAndSet(ui->DefaultProfile_Test_4_BlockSize, params.BlockSize);
        findDataAndSet(ui->DefaultProfile_Test_4_Queues, params.Queues);
        findDataAndSet(ui->DefaultProfile_Test_4_Threads, params.Threads);


        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Peak, Global::BenchmarkPreset::Standard);
        findDataAndSet(ui->PeakPerformanceProfile_Test_1_Pattern, params.Pattern);
        findDataAndSet(ui->PeakPerformanceProfile_Test_1_BlockSize, params.BlockSize);
        findDataAndSet(ui->PeakPerformanceProfile_Test_1_Queues, params.Queues);
        findDataAndSet(ui->PeakPerformanceProfile_Test_1_Threads, params.Threads);

        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Peak, Global::BenchmarkPreset::Standard);
        findDataAndSet(ui->PeakPerformanceProfile_Test_2_Pattern, params.Pattern);
        findDataAndSet(ui->PeakPerformanceProfile_Test_2_BlockSize, params.BlockSize);
        findDataAndSet(ui->PeakPerformanceProfile_Test_2_Queues, params.Queues);
        findDataAndSet(ui->PeakPerformanceProfile_Test_2_Threads, params.Threads);


        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Demo, Global::BenchmarkPreset::Standard);
        findDataAndSet(ui->DemoProfile_Test_1_Pattern, params.Pattern);
        findDataAndSet(ui->DemoProfile_Test_1_BlockSize, params.BlockSize);
        findDataAndSet(ui->DemoProfile_Test_1_Queues, params.Queues);
        findDataAndSet(ui->DemoProfile_Test_1_Threads, params.Threads);


        findDataAndSet(ui->MeasuringTime, settings.defaultMeasuringTime());
        findDataAndSet(ui->IntervalTime, settings.defaultIntervalTime());
    }
    else {
        Global::BenchmarkParams params;

        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Default, Global::BenchmarkPreset::NVMe_SSD);
        findDataAndSet(ui->DefaultProfile_Test_1_Pattern, params.Pattern);
        findDataAndSet(ui->DefaultProfile_Test_1_BlockSize, params.BlockSize);
        findDataAndSet(ui->DefaultProfile_Test_1_Queues, params.Queues);
        findDataAndSet(ui->DefaultProfile_Test_1_Threads, params.Threads);

        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Default, Global::BenchmarkPreset::NVMe_SSD);
        findDataAndSet(ui->DefaultProfile_Test_2_Pattern, params.Pattern);
        findDataAndSet(ui->DefaultProfile_Test_2_BlockSize, params.BlockSize);
        findDataAndSet(ui->DefaultProfile_Test_2_Queues, params.Queues);
        findDataAndSet(ui->DefaultProfile_Test_2_Threads, params.Threads);

        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_3, Global::PerformanceProfile::Default, Global::BenchmarkPreset::NVMe_SSD);
        findDataAndSet(ui->DefaultProfile_Test_3_Pattern, params.Pattern);
        findDataAndSet(ui->DefaultProfile_Test_3_BlockSize, params.BlockSize);
        findDataAndSet(ui->DefaultProfile_Test_3_Queues, params.Queues);
        findDataAndSet(ui->DefaultProfile_Test_3_Threads, params.Threads);

        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_4, Global::PerformanceProfile::Default, Global::BenchmarkPreset::NVMe_SSD);
        findDataAndSet(ui->DefaultProfile_Test_4_Pattern, params.Pattern);
        findDataAndSet(ui->DefaultProfile_Test_4_BlockSize, params.BlockSize);
        findDataAndSet(ui->DefaultProfile_Test_4_Queues, params.Queues);
        findDataAndSet(ui->DefaultProfile_Test_4_Threads, params.Threads);


        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Peak, Global::BenchmarkPreset::NVMe_SSD);
        findDataAndSet(ui->PeakPerformanceProfile_Test_1_Pattern, params.Pattern);
        findDataAndSet(ui->PeakPerformanceProfile_Test_1_BlockSize, params.BlockSize);
        findDataAndSet(ui->PeakPerformanceProfile_Test_1_Queues, params.Queues);
        findDataAndSet(ui->PeakPerformanceProfile_Test_1_Threads, params.Threads);

        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_2, Global::PerformanceProfile::Peak, Global::BenchmarkPreset::NVMe_SSD);
        findDataAndSet(ui->PeakPerformanceProfile_Test_2_Pattern, params.Pattern);
        findDataAndSet(ui->PeakPerformanceProfile_Test_2_BlockSize, params.BlockSize);
        findDataAndSet(ui->PeakPerformanceProfile_Test_2_Queues, params.Queues);
        findDataAndSet(ui->PeakPerformanceProfile_Test_2_Threads, params.Threads);


        params = settings.defaultBenchmarkParams(Global::BenchmarkTest::Test_1, Global::PerformanceProfile::Demo, Global::BenchmarkPreset::NVMe_SSD);
        findDataAndSet(ui->DemoProfile_Test_1_Pattern, params.Pattern);
        findDataAndSet(ui->DemoProfile_Test_1_BlockSize, params.BlockSize);
        findDataAndSet(ui->DemoProfile_Test_1_Queues, params.Queues);
        findDataAndSet(ui->DemoProfile_Test_1_Threads, params.Threads);


        findDataAndSet(ui->MeasuringTime, settings.defaultMeasuringTime());
        findDataAndSet(ui->IntervalTime, settings.defaultIntervalTime());
    }
}
