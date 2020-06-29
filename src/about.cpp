#include "about.h"
#include "ui_about.h"

#include "cmake.h"

#include <QAbstractButton>
#include <QIcon>

About::About(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::About)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->label_Version->setText(qApp->applicationVersion());

    setWindowIcon(QIcon("icons/kdiskmark.svg"));

    ui->label_Icon->setPixmap(QPixmap("icons/kdiskmark.png"));
}

About::~About()
{
    delete ui;
}

void About::setFIOVersion(const QString &version)
{
    ui->label_FIO->setText(version);
}

void About::on_buttonBox_clicked(QAbstractButton *)
{
    close();
}
