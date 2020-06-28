#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>

class QAbstractButton;

namespace Ui {
class About;
}

class About : public QDialog
{
    Q_OBJECT

public:
    explicit About(QWidget *parent = nullptr);
    ~About();

private slots:
    void on_buttonBox_clicked(QAbstractButton *);

private:
    Ui::About *ui;
};

#endif // ABOUT_H
