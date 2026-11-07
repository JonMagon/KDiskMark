#include "about.h"
#include "ui_about.h"

#include "cmake.h"
#include "global.h"

#include <QIcon>

About::About(const QString &FIOVersion, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->label_Version->setText(qApp->applicationVersion());
    ui->label_FIO->setText(FIOVersion);

#ifdef Q_OS_WIN
    ui->label_Icon->setPixmap(QPixmap(QStringLiteral(":/icons/kdiskmark.png")).scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
#else
    ui->label_Icon->setPixmap(QPixmap(qApp->applicationDirPath() + QStringLiteral("/../share/icons/hicolor/128x128/apps/kdiskmark.png")));
#endif
}

About::~About()
{
    delete ui;
}

void About::on_buttonBox_clicked(QAbstractButton *)
{
    close();
}
