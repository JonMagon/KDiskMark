#include "settings.h"
#include "ui_settings.h"

#include <QAbstractButton>
#include <QIcon>

#include "appsettings.h"
#include "global.h"

Settings::Settings(AppSettings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_settings = settings;

    QString i_str, j_str;

    for (int i = 1, j = 1; i <= 64; i++, (j <= 512 ? j *= 2 : j)) {
        i_str = QString::number(i);
        j_str = QString::number(j);

        if (j <= 8) {
            ui->SEQ_1_BlockSize->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("MiB")), j * 1024);
            ui->SEQ_2_BlockSize->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("MiB")), j * 1024);
        }

        if (j <= 16) {
            ui->SEQ_1_Queues->addItem(j_str, j);
            ui->SEQ_2_Queues->addItem(j_str, j);
        }

        if (j <= 512) {
            ui->RND_1_Queues->addItem(j_str, j);
            ui->RND_2_Queues->addItem(j_str, j);

            if (j >= 4) {
                ui->RND_1_BlockSize->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("KiB")), j);
                ui->RND_2_BlockSize->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("KiB")), j);
            }
        }

        ui->SEQ_1_Threads->addItem(i_str, i);
        ui->SEQ_2_Threads->addItem(i_str, i);
        ui->RND_1_Threads->addItem(i_str, i);
        ui->RND_2_Threads->addItem(i_str, i);
    }

    setActualValues();

    switch (m_settings->performanceProfile) {
    case AppSettings::PerformanceProfile::RealWorld:
        ui->SEQ_1_BlockSize->setEnabled(false);
        ui->SEQ_1_Queues->setEnabled(false);
        ui->SEQ_1_Threads->setEnabled(false);
        ui->RND_1_BlockSize->setEnabled(false);
        ui->RND_1_Queues->setEnabled(false);
        ui->RND_1_Threads->setEnabled(false);
        [[fallthrough]];
    case AppSettings::PerformanceProfile::Peak:
        ui->SEQ_2_BlockSize->setEnabled(false);
        ui->SEQ_2_Queues->setEnabled(false);
        ui->SEQ_2_Threads->setEnabled(false);
        ui->RND_2_BlockSize->setEnabled(false);
        ui->RND_2_Queues->setEnabled(false);
        ui->RND_2_Threads->setEnabled(false);
        break;
    }
}

Settings::~Settings()
{
    delete ui;
}

void Settings::findDataAndSet(QComboBox *comboBox, int data)
{
    comboBox->setCurrentIndex(comboBox->findData(data));
}

void Settings::setActualValues()
{
    AppSettings::BenchmarkParams params;

    params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1);
    findDataAndSet(ui->SEQ_1_BlockSize, params.BlockSize);
    findDataAndSet(ui->SEQ_1_Queues, params.Queues);
    findDataAndSet(ui->SEQ_1_Threads, params.Threads);

    params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2);
    findDataAndSet(ui->SEQ_2_BlockSize, params.BlockSize);
    findDataAndSet(ui->SEQ_2_Queues, params.Queues);
    findDataAndSet(ui->SEQ_2_Threads, params.Threads);

    params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_1);
    findDataAndSet(ui->RND_1_BlockSize, params.BlockSize);
    findDataAndSet(ui->RND_1_Queues, params.Queues);
    findDataAndSet(ui->RND_1_Threads, params.Threads);

    params = m_settings->getBenchmarkParams(AppSettings::BenchmarkTest::RND_2);
    findDataAndSet(ui->RND_2_BlockSize, params.BlockSize);
    findDataAndSet(ui->RND_2_Queues, params.Queues);
    findDataAndSet(ui->RND_2_Threads, params.Threads);
}

void Settings::on_buttonBox_clicked(QAbstractButton *button)
{
    if (ui->buttonBox->standardButton(button) == QDialogButtonBox::Ok) {
        m_settings->setBenchmarkParams(AppSettings::BenchmarkTest::SEQ_1,
                                       ui->SEQ_1_BlockSize->currentData().toInt(),
                                       ui->SEQ_1_Queues->currentData().toInt(),
                                       ui->SEQ_1_Threads->currentData().toInt());

        m_settings->setBenchmarkParams(AppSettings::BenchmarkTest::SEQ_2,
                                       ui->SEQ_2_BlockSize->currentData().toInt(),
                                       ui->SEQ_2_Queues->currentData().toInt(),
                                       ui->SEQ_2_Threads->currentData().toInt());

        m_settings->setBenchmarkParams(AppSettings::BenchmarkTest::RND_1,
                                       ui->RND_1_BlockSize->currentData().toInt(),
                                       ui->RND_1_Queues->currentData().toInt(),
                                       ui->RND_1_Threads->currentData().toInt());

        m_settings->setBenchmarkParams(AppSettings::BenchmarkTest::RND_2,
                                       ui->RND_2_BlockSize->currentData().toInt(),
                                       ui->RND_2_Queues->currentData().toInt(),
                                       ui->RND_2_Threads->currentData().toInt());

        close();
    }
    else if (ui->buttonBox->standardButton(button) == QDialogButtonBox::RestoreDefaults) {
        m_settings->restoreDefaultBenchmarkParams();

        setActualValues();
    }
}
