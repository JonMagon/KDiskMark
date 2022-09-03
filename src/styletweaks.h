#ifndef STYLETWEAKS_H
#define STYLETWEAKS_H

#include <QProxyStyle>
#include <QStyleOption>

class StyleTweaks : public QProxyStyle
{
public:
    virtual void drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    virtual void drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = nullptr) const override;
};

#endif // STYLETWEAKS_H
