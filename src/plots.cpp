#include "plots.h"
#include "ui_plots.h"

#include "qcustomplot/qcustomplot.h"

Plots::Plots(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Plots)
{
    ui->setupUi(this);

    auto dataPlot = [&] (const QString &fileName, QVector<double> &x, QVector<double> &y, float &average) {
        x.clear(); y.clear();
        QFile inputFile(fileName);
        if (inputFile.open(QIODevice::ReadOnly)) {
           QTextStream in(&inputFile);
           while (!in.atEnd()) {
              QStringList components = in.readLine().split(QLatin1Char(','));
              x.append(components[0].trimmed().toInt() / 1000.);
              y.append(components[1].trimmed().toInt());
           }
           inputFile.close();
        }
        average = std::accumulate(y.begin(), y.end(), .0) / y.size();
    };

    QVector<double> x, y;
    float average;

    QCustomPlot *plot;

    // Bandwidth
    {
        dataPlot("/tmp/.kdiskmark_bw.1.log", x, y, average);

        plot = ui->bandwidthPlot;
        plot->plotLayout()->insertRow(0);
        plot->plotLayout()->addElement(0, 0, new QCPTextElement(plot, QStringLiteral("Throughput (KiB/s), Average = %1").arg(average), QFont("Helvetica", 12)));

        plot->xAxis->setLabel("Time (Seconds)");
        plot->yAxis->setLabel("KiB/s");

        plot->addGraph();
        plot->graph()->setData(x, y);

        plot->rescaleAxes();

        QCPItemLine *line = new QCPItemLine(plot);
        line->setPen(QPen(Qt::green, 2));
        line->start->setCoords(0, average);
        line->end->setCoords(plot->xAxis->range().maxRange, average);
    }

    // IOPS
    {
        dataPlot("/tmp/.kdiskmark_iops.1.log", x, y, average);

        plot = ui->IOPSPlot;
        plot->plotLayout()->insertRow(0);
        plot->plotLayout()->addElement(0, 0, new QCPTextElement(plot, QStringLiteral("IOPS, Average = %1").arg(average), QFont("Helvetica", 12)));

        plot->xAxis->setLabel("Time (Seconds)");
        plot->yAxis->setLabel("IOPS");

        plot->addGraph();
        plot->graph()->setData(x, y);

        plot->rescaleAxes();

        QCPItemLine *line = new QCPItemLine(plot);
        line->setPen(QPen(Qt::green, 2));
        line->start->setCoords(0, average);
        line->end->setCoords(plot->xAxis->range().maxRange, average);
    }

    // Latency
    {
        dataPlot("/tmp/.kdiskmark_lat.1.log", x, y, average);

        plot = ui->latencyPlot;
        plot->plotLayout()->insertRow(0);
        plot->plotLayout()->addElement(0, 0, new QCPTextElement(plot, QStringLiteral("Latency, Average = %1").arg(average), QFont("Helvetica", 12)));

        plot->xAxis->setLabel("Time (Seconds)");
        plot->yAxis->setLabel("Latency");

        plot->addGraph();
        plot->graph()->setData(x, y);

        plot->rescaleAxes();

        QCPItemLine *line = new QCPItemLine(plot);
        line->setPen(QPen(Qt::green, 2));
        line->start->setCoords(0, average);
        line->end->setCoords(plot->xAxis->range().maxRange, average);
    }
}

Plots::~Plots()
{
    delete ui;
}
