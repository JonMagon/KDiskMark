#include "styletweaks.h"

void StyleTweaks::drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    const QStyleOptionButton *buttonWidget;

    if (element == QStyle::CE_PushButton && (buttonWidget = qstyleoption_cast<const QStyleOptionButton *>(option))) {
        QStyleOptionButton copy = *buttonWidget;
        copy.state &= ~QStyle::State_HasFocus;
        QProxyStyle::drawControl(element, &copy, painter, widget);
        return;
    }

    const QStyleOptionComboBox *comboboxWidget;

    if (element == QStyle::CE_ComboBoxLabel && (comboboxWidget = qstyleoption_cast<const QStyleOptionComboBox *>(option))) {
        QStyleOptionComboBox copy = *comboboxWidget;
        copy.state &= ~QStyle::State_HasFocus;
        QProxyStyle::drawControl(element, &copy, painter, widget);
        return;
    }

    QProxyStyle::drawControl(element, option, painter, widget);
}

void StyleTweaks::drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const
{
    const QStyleOptionComboBox *comboboxWidget;

    if (control == QStyle::CC_ComboBox && (comboboxWidget = qstyleoption_cast<const QStyleOptionComboBox *>(option))) {
        QStyleOptionComboBox copy = *comboboxWidget;
        copy.state &= ~QStyle::State_HasFocus;
        QProxyStyle::drawComplexControl(control, &copy, painter, widget);
        return;
    }

    QProxyStyle::drawComplexControl(control, option, painter, widget);
}
