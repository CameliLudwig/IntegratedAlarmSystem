#include "gaugeCompass.h"
#include <QPainter>
#include <QLinearGradient>
#include <QTimer>
#include <QtMath>
#include <cmath>

namespace {
double normalizedAngle(double angle)
{
    angle = std::fmod(angle, 360.0);
    return angle < 0.0 ? angle + 360.0 : angle;
}
}

GaugeCompass::GaugeCompass(QWidget *parent)
    : QWidget(parent),
      value(0.0),
      precision(0),
      animation(false),
      animationStep(3.0),
      crownColorStart(90, 90, 90),
      crownColorEnd(25, 25, 25),
      bgColorStart(55, 65, 75),
      bgColorEnd(20, 25, 30),
      darkColor(35, 35, 35),
      lightColor(220, 220, 220),
      foreground(235, 235, 235),
      textColor(255, 255, 255),
      northPointerColor(225, 65, 55),
      southPointerColor(235, 235, 235),
      centerColorStart(245, 195, 70),
      centerColorEnd(135, 85, 25),
      currentValue(0.0),
      timer(new QTimer(this))
{
    setMinimumSize(140, 140);
    timer->setInterval(16);
    connect(timer, &QTimer::timeout, this, &GaugeCompass::updateValue);
}

GaugeCompass::~GaugeCompass() = default;

double GaugeCompass::getValue() const { return value; }
int GaugeCompass::getPrecision() const { return precision; }
bool GaugeCompass::getAnimation() const { return animation; }
double GaugeCompass::getAnimationStep() const { return animationStep; }
QColor GaugeCompass::getCrownColorStart() const { return crownColorStart; }
QColor GaugeCompass::getCrownColorEnd() const { return crownColorEnd; }
QColor GaugeCompass::getBgColorStart() const { return bgColorStart; }
QColor GaugeCompass::getBgColorEnd() const { return bgColorEnd; }
QColor GaugeCompass::getDarkColor() const { return darkColor; }
QColor GaugeCompass::getLightColor() const { return lightColor; }
QColor GaugeCompass::getForeground() const { return foreground; }
QColor GaugeCompass::getTextColor() const { return textColor; }
QColor GaugeCompass::getNorthPointerColor() const { return northPointerColor; }
QColor GaugeCompass::getSouthPointerColor() const { return southPointerColor; }
QColor GaugeCompass::getCenterColorStart() const { return centerColorStart; }
QColor GaugeCompass::getCenterColorEnd() const { return centerColorEnd; }

QSize GaugeCompass::sizeHint() const
{
    return QSize(220, 220);
}

QSize GaugeCompass::minimumSizeHint() const
{
    return QSize(140, 140);
}

void GaugeCompass::setValue(double newValue)
{
    if (!std::isfinite(newValue)) {
        return;
    }

    newValue = normalizedAngle(newValue);
    if (qFuzzyCompare(value + 1.0, newValue + 1.0)) {
        return;
    }

    value = newValue;
    int signalValue = qRound(value);
    if (signalValue >= 360) {
        signalValue = 0;
    }
    emit valueChanged(signalValue);

    if (animation) {
        timer->start();
    } else {
        currentValue = value;
        timer->stop();
        update();
    }
}

void GaugeCompass::setValue(int newValue)
{
    setValue(static_cast<double>(newValue));
}

void GaugeCompass::setPrecision(int newPrecision)
{
    precision = qBound(0, newPrecision, 6);
    update();
}

void GaugeCompass::setAnimation(bool enabled)
{
    if (animation == enabled) {
        return;
    }

    animation = enabled;
    if (animation) {
        if (!qFuzzyCompare(currentValue + 1.0, value + 1.0)) {
            timer->start();
        }
    } else {
        timer->stop();
        currentValue = value;
        update();
    }
}

void GaugeCompass::setAnimationStep(double step)
{
    if (!std::isfinite(step)) {
        return;
    }
    animationStep = qBound(0.1, step, 360.0);
}

void GaugeCompass::setCrownColorStart(const QColor &color) { crownColorStart = color; update(); }
void GaugeCompass::setCrownColorEnd(const QColor &color) { crownColorEnd = color; update(); }
void GaugeCompass::setBgColorStart(const QColor &color) { bgColorStart = color; update(); }
void GaugeCompass::setBgColorEnd(const QColor &color) { bgColorEnd = color; update(); }
void GaugeCompass::setDarkColor(const QColor &color) { darkColor = color; update(); }
void GaugeCompass::setLightColor(const QColor &color) { lightColor = color; update(); }
void GaugeCompass::setForeground(const QColor &color) { foreground = color; update(); }
void GaugeCompass::setTextColor(const QColor &color) { textColor = color; update(); }
void GaugeCompass::setNorthPointerColor(const QColor &color) { northPointerColor = color; update(); }
void GaugeCompass::setSouthPointerColor(const QColor &color) { southPointerColor = color; update(); }
void GaugeCompass::setCenterColorStart(const QColor &color) { centerColorStart = color; update(); }
void GaugeCompass::setCenterColorEnd(const QColor &color) { centerColorEnd = color; update(); }

void GaugeCompass::updateValue()
{
    double difference = normalizedAngle(value - currentValue);
    if (std::fabs(difference) <= animationStep) {
        currentValue = value;
        timer->stop();
    } else {
        currentValue = normalizedAngle(currentValue + animationStep);
    }
    update();
}

void GaugeCompass::paintEvent(QPaintEvent *)
{
    int width = this->width();
    int height = this->height();
    int side = qMin(width, height);

    //绘制准备工作,启用反锯齿,平移坐标轴中心,等比例缩放
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.translate(width / 2, height / 2);
    painter.scale(side / 200.0, side / 200.0);

    //绘制外边框圆
    drawCrownCircle(&painter);
    //绘制背景圆
    drawBgCircle(&painter);
    //绘制刻度
    drawScale(&painter);
    //绘制东南西北标识
    drawScaleNum(&painter);
    //绘制覆盖圆外圆
    drawCoverOuterCircle(&painter);
    //绘制覆盖圆内圆
    // drawCoverInnerCircle(&painter);
    //绘制覆盖圆中心圆
    drawCoverCenterCircle(&painter);
    //绘制南北指针
    drawPointer(&painter);
    //绘制中心圆
    drawCenterCircle(&painter);
    //绘制当前值
    drawValue(&painter);
}

void GaugeCompass::drawCrownCircle(QPainter *painter)
{
    int radius = 99;
    painter->save();
    painter->setPen(Qt::NoPen);
    QLinearGradient lineGradient(0, -radius, 0, radius);
    lineGradient.setColorAt(0, crownColorStart);
    lineGradient.setColorAt(1, crownColorEnd);
    painter->setBrush(lineGradient);
    painter->drawEllipse(-radius, -radius, radius * 2, radius * 2);
    painter->restore();
}

void GaugeCompass::drawBgCircle(QPainter *painter)
{
    int radius = 90;
    painter->save();
    painter->setPen(Qt::NoPen);
    QLinearGradient lineGradient(0, -radius, 0, radius);
    lineGradient.setColorAt(0, bgColorStart);
    lineGradient.setColorAt(1, bgColorEnd);
    painter->setBrush(lineGradient);
    painter->drawEllipse(-radius, -radius, radius * 2, radius * 2);
    painter->restore();
}

void GaugeCompass::drawScale(QPainter *painter)
{
    int radius = 85;
    painter->save();

    //总共8格,4格为NESW字母,4格为线条
    int steps = 8;
    double angleStep = 360.0 / steps;

    QPen pen;
    pen.setColor(foreground);
    pen.setCapStyle(Qt::RoundCap);
    pen.setWidth(4);
    painter->setPen(pen);

    //%2整数部分绘制NESW字母,其余绘制线条刻度
    for (int i = 0; i <= steps; i++) {
        if (i % 2 != 0) {
            painter->drawLine(0, radius - 10, 0, radius);
        }

        painter->rotate(angleStep);
    }

    painter->restore();
}

void GaugeCompass::drawScaleNum(QPainter *painter)
{
    int radius = 88;
    painter->save();
    painter->setPen(foreground);

    QFont font;
    font.setPixelSize(9);
    font.setBold(true);
    painter->setFont(font);

    QRect textRect = QRect(-radius, -radius, radius * 2, radius * 2);
    painter->drawText(textRect, Qt::AlignTop | Qt::AlignHCenter, "0");
    painter->drawText(textRect, Qt::AlignBottom | Qt::AlignHCenter, "180");

    radius -= 2;
    textRect = QRect(-radius, -radius, radius * 2, radius * 2);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, "270");

    radius -= 2;
    textRect = QRect(-radius, -radius, radius * 2, radius * 2);
    painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, "90");

    painter->restore();
}

void GaugeCompass::drawCoverOuterCircle(QPainter *painter)
{
    int radius = 68;
    painter->save();
    painter->setPen(Qt::NoPen);
    QLinearGradient lineGradient(0, -radius, 0, radius);
    lineGradient.setColorAt(0, lightColor);
    lineGradient.setColorAt(1, darkColor);
    painter->setBrush(lineGradient);
    painter->drawEllipse(-radius, -radius, radius * 2, radius * 2);
    painter->restore();
}

void GaugeCompass::drawCoverInnerCircle(QPainter *painter)
{
    int radius = 60;
    painter->save();
    painter->setPen(Qt::NoPen);
    QLinearGradient lineGradient(0, -radius, 0, radius);
    lineGradient.setColorAt(0, darkColor);
    lineGradient.setColorAt(1, lightColor);
    painter->setBrush(lineGradient);
    painter->drawEllipse(-radius, -radius, radius * 2, radius * 2);
    painter->restore();
}

void GaugeCompass::drawCoverCenterCircle(QPainter *painter)
{
    int radius = 15;
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setOpacity(0.8);
    QLinearGradient lineGradient(0, -radius, 0, radius);
    lineGradient.setColorAt(0, lightColor);
    lineGradient.setColorAt(1, darkColor);
    painter->setBrush(lineGradient);
    painter->drawEllipse(-radius, -radius, radius * 2, radius * 2);
    painter->restore();
}

void GaugeCompass::drawPointer(QPainter *painter)
{
    int radius = 75;

    QPolygon pts;

    painter->save();
    painter->setOpacity(0.7);
    painter->setPen(Qt::NoPen);
    painter->setBrush(northPointerColor);
    pts.setPoints(3, -10, 0, 10, 0, 0, radius);
    painter->rotate(currentValue + 180);
    painter->drawConvexPolygon(pts);
    painter->restore();

    // painter->save();
    // painter->setOpacity(0.7);
    // painter->setPen(Qt::NoPen);
    // painter->setBrush(southPointerColor);
    // pts.setPoints(3, -10, 0, 10, 0, 0, radius);
    // painter->rotate(currentValue);
    // painter->drawConvexPolygon(pts);
    // painter->restore();
}

void GaugeCompass::drawCenterCircle(QPainter *painter)
{
    int radius = 12;
    painter->save();
    painter->setOpacity(1.0);
    painter->setPen(Qt::NoPen);
    QLinearGradient lineGradient(0, -radius, 0, radius);
    lineGradient.setColorAt(0, centerColorStart);
    lineGradient.setColorAt(1, centerColorEnd);
    painter->setBrush(lineGradient);
    painter->drawEllipse(-radius, -radius, radius * 2, radius * 2);
    painter->restore();
}

void GaugeCompass::drawValue(QPainter *painter)
{
    int radius = 100;
    painter->save();
    painter->setPen(textColor);

    QFont font;
    font.setPixelSize(11);
    font.setBold(true);
    painter->setFont(font);

    QRectF textRect(-radius, -radius, radius * 2, radius * 2);
    QString strValue = QString("%1").arg((double)value, 0, 'f', precision);
    painter->drawText(textRect, Qt::AlignCenter, strValue);
    painter->restore();
}
