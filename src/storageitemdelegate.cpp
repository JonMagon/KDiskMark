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
        QStyledItemDelegate::paint(painter, option, QModelIndex());

        Global::Storage storage = variant.value<Global::Storage>();

        QStyle *style = option.widget ? option.widget->style() : QApplication::style();

        QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                                  ? QPalette::Normal : QPalette::Disabled;
        if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
            cg = QPalette::Inactive;

        if (option.state & QStyle::State_Selected) {
            //painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
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
        const int textVMargin = style->pixelMetric(QStyle::PM_FocusFrameVMargin, 0, option.widget) + 1;

        progressBarOption.rect = progressBarRect.adjusted(0, textHeight / 2 + textVMargin, -percentTextWidth, -textHeight / 2 - textVMargin);
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
    else if (index.data(Qt::DecorationRole).type() == QVariant::Icon) {
        QStyledItemDelegate::paint(painter, option, QModelIndex());

        QStyle *style = option.widget ? option.widget->style() : QApplication::style();

        QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                                  ? QPalette::Normal : QPalette::Disabled;
        if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
            cg = QPalette::Inactive;

        if (option.state & QStyle::State_Selected) {
            //painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
            painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
        } else {
            painter->setPen(option.palette.color(cg, QPalette::Text));
        }

        QString text = index.data(Qt::DisplayRole).toString();

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        int textWidth = QFontMetrics(painter->font()).horizontalAdvance(text);
#else
        int textWidth = QFontMetrics(painter->font()).width(text);
#endif

        const int textMargin = (style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, option.widget) + 1);
        int textMarginCenter = (option.rect.size().width() - (option.decorationSize.width() + textMargin * 8 + textWidth)) / 2;

        auto opt = option;
        if (!(opt.state & QStyle::State_HasFocus))
            opt.state &= ~(QStyle::State_MouseOver);
        opt.rect = opt.rect.adjusted(textMarginCenter, 0, -textMarginCenter, 0);

        QPalette palette = opt.palette;
        palette.setColor(QPalette::Text, painter->pen().color());
        opt.palette = palette;

        QStyledItemDelegate::paint(painter, opt, index);
    }
    else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}
