#ifndef STYLETWEAKS_H
#define STYLETWEAKS_H

#include <QProxyStyle>
#include <QStyleOption>

class StyleTweaks : public QProxyStyle
{
public:
#ifdef Q_OS_WIN
    int styleHint(QStyle::StyleHint hint, const QStyleOption *option = nullptr, const QWidget *widget = nullptr, QStyleHintReturn *returnData = nullptr) const override;
#endif
    virtual void drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
    virtual void drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget = nullptr) const override;
};

#endif // STYLETWEAKS_H
