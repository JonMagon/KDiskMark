#ifndef STORAGEITEMDELEGATE_H
#define STORAGEITEMDELEGATE_H

#include <QStyledItemDelegate>

class StorageItemDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    StorageItemDelegate(QObject *parent = nullptr);

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // STORAGEITEMDELEGATE_H
