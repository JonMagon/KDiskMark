#include "storageitemdelegate.h"

#include <QPainter>
#include <QApplication>

#include "global.h"

StorageItemDelegate::StorageItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

QSize StorageItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
    s.setHeight(s.height() * 2); // double item height
    return s;
}

void StorageItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant variant = index.data(Qt::UserRole);

    if (variant.canConvert<Global::Storage>()) {
        Global::Storage storage = variant.value<Global::Storage>();

        QStyle *style = option.widget ? option.widget->style() : QApplication::style();

        QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                                  ? QPalette::Normal : QPalette::Disabled;
        if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
            cg = QPalette::Inactive;

        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
            painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
        } else {
            painter->setPen(option.palette.color(cg, QPalette::Text));
        }

        const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, option.widget) + 1;
        // we will move the text up half of the total height
        const int topMargin = QStyledItemDelegate::sizeHint(option, index).height() / 2;
        QRect textRect = option.rect.adjusted(textMargin, -topMargin, -textMargin, -topMargin);

        painter->drawText(textRect, option.displayAlignment, storage.path);
        painter->drawText(textRect, option.displayAlignment | Qt::AlignRight, storage.formatedSize);

        QStyleOptionProgressBar progressBarOption;

        const int textHeight = QFontMetrics(painter->font()).height();
        QRect progressBarRect = textRect.adjusted(0, textHeight, 0, textHeight);

        const int percent = storage.bytesOccupied * 100 / storage.bytesTotal;

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        int percentTextWidth = QFontMetrics(painter->font()).horizontalAdvance(QLatin1Char('0')) * 4;
#else
        int percentTextWidth = QFontMetrics(painter->font()).width(QLatin1Char('0')) * 4;
#endif
        progressBarOption.rect = progressBarRect.adjusted(0, 0, -percentTextWidth, 0);
        progressBarOption.minimum = 0;
        progressBarOption.maximum = 100;
        progressBarOption.progress = percent;

        if (percent >= 95) {
            QPalette palette = progressBarOption.palette;
            palette.setColor(QPalette::Highlight, QColor(218, 68, 83));
            progressBarOption.palette = palette;
        }

        style->drawControl(QStyle::CE_ProgressBar, &progressBarOption, painter);

        painter->drawText(progressBarRect, option.displayAlignment | Qt::AlignRight, QString::number(percent) + "%");
    }
    else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}
