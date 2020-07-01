#include "settings.h"
#include "ui_settings.h"

#include "appsettings.h"

Settings::Settings(AppSettings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);

    m_settings = settings;

    QString i_str, j_str;
    for (int i = 1, j = 1; i <= 64; i++, (j <= 512 ? j *= 2 : j)) {

        i_str = QString::number(i);
        j_str = QString::number(j);

        if (j <= 8) {
            ui->comboBox_BlockSize_SEQ_1->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("MiB")), j * 1024);
            ui->comboBox_BlockSize_SEQ_2->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("MiB")), j * 1024);
        }

        if (j <= 16) {
            ui->comboBox_Queues_SEQ_1->addItem(j_str, j);
            ui->comboBox_Queues_SEQ_2->addItem(j_str, j);
        }

        if (j <= 512) {
            ui->comboBox_Queues_RND_1->addItem(j_str, j);
            ui->comboBox_Queues_RND_2->addItem(j_str, j);

            if (j >= 4) {
                ui->comboBox_BlockSize_RND_1->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("KiB")), j);
                ui->comboBox_BlockSize_RND_2->addItem(QStringLiteral("%1 %2").arg(j).arg(tr("KiB")), j);
            }
        }

        ui->comboBox_Threads_SEQ_1->addItem(i_str, i);
        ui->comboBox_Threads_SEQ_2->addItem(i_str, i);
        ui->comboBox_Threads_RND_1->addItem(i_str, i);
        ui->comboBox_Threads_RND_2->addItem(i_str, i);
    }

    ui->comboBox_BlockSize_SEQ_1->
            setCurrentIndex(ui->comboBox_BlockSize_SEQ_1->findData(m_settings->SEQ_1.BlockSize));
    ui->comboBox_BlockSize_SEQ_2->
            setCurrentIndex(ui->comboBox_BlockSize_SEQ_2->findData(m_settings->SEQ_2.BlockSize));
    ui->comboBox_BlockSize_RND_1->
            setCurrentIndex(ui->comboBox_BlockSize_RND_1->findData(m_settings->RND_1.BlockSize));
    ui->comboBox_BlockSize_RND_2->
            setCurrentIndex(ui->comboBox_BlockSize_RND_2->findData(m_settings->RND_2.BlockSize));

    ui->comboBox_Queues_SEQ_1->
            setCurrentIndex(ui->comboBox_Queues_SEQ_1->findData(m_settings->SEQ_1.Queues));
    ui->comboBox_Queues_SEQ_2->
            setCurrentIndex(ui->comboBox_Queues_SEQ_2->findData(m_settings->SEQ_2.Queues));
    ui->comboBox_Queues_RND_1->
            setCurrentIndex(ui->comboBox_Queues_RND_1->findData(m_settings->RND_1.Queues));
    ui->comboBox_Queues_RND_2->
            setCurrentIndex(ui->comboBox_Queues_RND_2->findData(m_settings->RND_2.Queues));

    ui->comboBox_Threads_SEQ_1->
            setCurrentIndex(ui->comboBox_Threads_SEQ_1->findData(m_settings->SEQ_1.Threads));
    ui->comboBox_Threads_SEQ_2->
            setCurrentIndex(ui->comboBox_Threads_SEQ_2->findData(m_settings->SEQ_2.Threads));
    ui->comboBox_Threads_RND_1->
            setCurrentIndex(ui->comboBox_Threads_RND_1->findData(m_settings->RND_1.Threads));
    ui->comboBox_Threads_RND_2->
            setCurrentIndex(ui->comboBox_Threads_RND_2->findData(m_settings->RND_2.Threads));
}

Settings::~Settings()
{
    delete ui;
}
