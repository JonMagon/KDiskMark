#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>

class AppSettings;
class QAbstractButton;
class QComboBox;

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
    Q_OBJECT

public:
    explicit Settings(QWidget *parent = nullptr);
    ~Settings();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::Settings *ui;
    void findDataAndSet(QComboBox* comboBox, int data);
};

#endif // SETTINGS_H
