#ifndef GLOBAL_H
#define GLOBAL_H

class QString;

namespace Global {
    int getOutputColumnsCount();
    QString getToolTipTemplate();
    QString getComparisonLabelTemplate();
    QString getRWSequentialRead();
    QString getRWSequentialWrite();
    QString getRWSequentialMix();
    QString getRWRandomRead();
    QString getRWRandomWrite();
    QString getRWRandomMix();
}

#endif // GLOBAL_H
