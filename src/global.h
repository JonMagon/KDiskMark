#ifndef GLOBAL_H
#define GLOBAL_H

class QString;

namespace Global {
    int getOutputColumnsCount();
    QString getToolTipTemplate();
    QString getComparisonLabelTemplate();
    QString getRWSequentialRead();
    QString getRWSequentialWrite();
    QString getRWRandomRead();
    QString getRWRandomWrite();
}

#endif // GLOBAL_H
