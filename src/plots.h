#ifndef PLOT_H
#define PLOT_H

#include <QDialog>

namespace Ui {
class Plots;
}

class Plots: public QDialog
{
    Q_OBJECT

public:
    explicit Plots(QWidget *parent = nullptr);
    ~Plots();

private:
    Ui::Plots *ui;
};

#endif // PLOT_H
