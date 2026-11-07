#include "storageitemdelegate.h"

#include <QPainter>
#include <QApplication>
#ifdef Q_OS_WIN
#include <QPainterPath>
#endif

#include "global.h"

StorageItemDelegate::StorageItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

QSize StorageItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize s = QStyledItemDelegate::sizeHint(option, index);
#ifdef Q_OS_WIN
    s.setHeight(qMax(s.height() * 2, option.fontMetrics.height() * 2 + 16));
#else
    s.setHeight(s.height() * 2); // double item height
#endif
    return s;
}

void StorageItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
#ifdef Q_OS_WIN
    painter->save();
    painter->setClipRect(option.rect);
    painter->setFont(option.font);
#endif
    QVariant variant = index.data(Qt::UserRole);

    if (variant.canConvert<Global::Storage>()) {
        Global::Storage storage = variant.value<Global::Storage>();

        QStyle *style = option.widget ? option.widget->style() : QApplication::style();

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.text = QString();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, option.widget);

        QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                                  ? QPalette::Normal : QPalette::Disabled;
        if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
            cg = QPalette::Inactive;

        if (option.state & QStyle::State_Selected) {
#ifdef Q_OS_WIN
            painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
#else
            //painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
#endif
            painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
        } else {
            painter->setPen(option.palette.color(cg, QPalette::Text));
        }

#ifdef Q_OS_WIN
        const int textMargin = qMax(6, style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, option.widget) + 1);
        const int textHeight = option.fontMetrics.height();
        const QRect content = option.rect.adjusted(textMargin, 6, -textMargin, -6);
        const QRect textRect(content.left(), content.top(), content.width(), textHeight);
        const int capacityWidth = qMin(textRect.width() / 2, option.fontMetrics.horizontalAdvance(storage.formatedSize));
        const QRect capacityRect(textRect.right() - capacityWidth + 1, textRect.top(), capacityWidth, textHeight);
        const QRect pathRect(textRect.left(), textRect.top(), qMax(0, textRect.width() - capacityWidth - 12), textHeight);
        painter->drawText(pathRect, Qt::AlignLeft | Qt::AlignVCenter,
                          option.fontMetrics.elidedText(storage.path, Qt::ElideMiddle, pathRect.width()));
        painter->drawText(capacityRect, Qt::AlignRight | Qt::AlignVCenter,
                          option.fontMetrics.elidedText(storage.formatedSize, Qt::ElideRight, capacityRect.width()));
#else
        const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, option.widget) + 1;
        // we will move the text up half of the total height
        const int topMargin = QStyledItemDelegate::sizeHint(option, index).height() / 2;
        QRect textRect = option.rect.adjusted(textMargin, -topMargin, -textMargin, -topMargin);
#endif

#ifdef Q_OS_WIN
        const QRect progressBarRect(content.left(), content.bottom() - textHeight + 1, content.width(), textHeight);
        const int percent = storage.bytesTotal > 0
            ? qBound(0, int(100.0 * storage.bytesOccupied / storage.bytesTotal), 100) : 0;
#else
        painter->drawText(textRect, option.displayAlignment, storage.path);
        painter->drawText(textRect, option.displayAlignment | Qt::AlignRight, storage.formatedSize);

        QStyleOptionProgressBar progressBarOption;

        const int textHeight = QFontMetrics(painter->font()).height();
        QRect progressBarRect = textRect.adjusted(0, textHeight, 0, textHeight);

        const int percent = storage.bytesOccupied * 100 / storage.bytesTotal;
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        int percentTextWidth = QFontMetrics(painter->font()).horizontalAdvance(QLatin1Char('0')) * 4;
#else
        int percentTextWidth = QFontMetrics(painter->font()).width(QLatin1Char('0')) * 4;
#endif

#ifdef Q_OS_WIN
        const int barHeight = qBound(10, textHeight - 3, 12);
        const QRectF frame(progressBarRect.left() + 0.5,
                           progressBarRect.center().y() - barHeight / 2 + 0.5,
                           qMax(0, progressBarRect.width() - percentTextWidth - 7), barHeight);
        const bool dark = option.palette.color(cg, QPalette::Base).lightness() < 128;
        const bool selected = option.state & QStyle::State_Selected;
        const QColor accent = option.palette.color(cg, QPalette::Highlight);
        const QColor track = selected ? accent.darker(135)
            : (dark ? QColor(52, 55, 60) : QColor(231, 234, 237));
        const QColor border = selected ? accent.lighter(125)
            : (dark ? QColor(91, 96, 104) : QColor(188, 194, 200));
        const QColor fill = percent >= 95 ? QColor(218, 68, 83)
            : (selected ? accent.lighter(155) : accent);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(track);
        painter->drawRoundedRect(frame, 3, 3);
        const QRectF inner = frame.adjusted(1, 1, -1, -1);
        if (percent > 0 && inner.width() > 0) {
            QPainterPath clip;
            clip.addRoundedRect(inner, 2, 2);
            painter->save();
            painter->setClipPath(clip, Qt::IntersectClip);
            QRectF filled = inner;
            filled.setWidth(inner.width() * percent / 100.0);
            if (option.direction == Qt::RightToLeft)
                filled.moveRight(inner.right());
            QLinearGradient gradient(inner.topLeft(), inner.bottomLeft());
            gradient.setColorAt(0, fill.lighter(108));
            gradient.setColorAt(1, fill);
            painter->setBrush(gradient);
            painter->drawRoundedRect(filled, 2, 2);
            painter->restore();
        }
        painter->setPen(QPen(border, 1));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(frame, 3, 3);
        painter->restore();
#else
        const int textVMargin = style->pixelMetric(QStyle::PM_FocusFrameVMargin, 0, option.widget) + 1;
#endif

#ifdef Q_OS_WIN
        painter->setPen(option.palette.color(cg, option.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text));
        painter->drawText(progressBarRect, Qt::AlignVCenter | Qt::AlignRight,
                          storage.bytesTotal > 0 ? QString::number(percent) + "%" : QStringLiteral("—"));
#else
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
#endif
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    else if (index.data(Qt::DecorationRole).typeId() == QVariant::Icon) {
#else
    else if (index.data(Qt::DecorationRole).type() == QVariant::Icon) {
#endif
        QStyle *style = option.widget ? option.widget->style() : QApplication::style();

        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.icon = QIcon();
        opt.text = QString();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, option.widget);

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
#ifdef Q_OS_WIN
    painter->restore();
#endif
}
