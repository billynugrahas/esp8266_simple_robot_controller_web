#include "L293D.h"

L293D::L293D()
    : _leftCoef(1.0f), _rightCoef(1.0f), _begun(false) {
}

void L293D::begin(L293DConfig config) {
    _cfg = config;

    pinMode(_cfg.leftDir, OUTPUT);
    pinMode(_cfg.leftPWM, OUTPUT);
    pinMode(_cfg.rightDir, OUTPUT);
    pinMode(_cfg.rightPWM, OUTPUT);

    analogWriteRange(_cfg.maxPWM);

    stop();
    _begun = true;
}

void L293D::setCoefficients(float left, float right) {
    _leftCoef = left;
    _rightCoef = right;
}

void L293D::setLeftSpeed(int speed) {
    if (!_begun) return;
    writeMotor(_cfg.leftDir, _cfg.leftPWM, speed, _leftCoef);
}

void L293D::setRightSpeed(int speed) {
    if (!_begun) return;
    writeMotor(_cfg.rightDir, _cfg.rightPWM, speed, _rightCoef);
}

void L293D::forward(int speed) {
    setLeftSpeed(speed);
    setRightSpeed(speed);
}

void L293D::backward(int speed) {
    setLeftSpeed(-speed);
    setRightSpeed(-speed);
}

void L293D::turnLeft(int speed) {
    setLeftSpeed(-speed);
    setRightSpeed(speed);
}

void L293D::turnRight(int speed) {
    setLeftSpeed(speed);
    setRightSpeed(-speed);
}

void L293D::stop() {
    if (!_begun) return;
    writeMotor(_cfg.leftDir, _cfg.leftPWM, 0, _leftCoef);
    writeMotor(_cfg.rightDir, _cfg.rightPWM, 0, _rightCoef);
}

void L293D::drive(const String& direction, int speed) {
    if (direction == "forward") forward(speed);
    else if (direction == "back") backward(speed);
    else if (direction == "left") turnLeft(speed);
    else if (direction == "right") turnRight(speed);
    else stop();
}

void L293D::writeMotor(int dirPin, int pwmPin, int speed, float coef) {
    if (speed > 0) {
        digitalWrite(dirPin, HIGH);
        analogWrite(pwmPin, (int)(speed / 100.0f * _cfg.maxPWM * coef));
    } else if (speed < 0) {
        digitalWrite(dirPin, LOW);
        analogWrite(pwmPin, (int)(-speed / 100.0f * _cfg.maxPWM * coef));
    } else {
        digitalWrite(dirPin, LOW);
        analogWrite(pwmPin, 0);
    }
}
