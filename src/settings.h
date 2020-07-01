#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>

class AppSettings;

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
    Q_OBJECT

    AppSettings *m_settings;

public:
    explicit Settings(AppSettings *settings, QWidget *parent = nullptr);
    ~Settings();

private:
    Ui::Settings *ui;
};

#endif // SETTINGS_H
