#pragma once
#include <Arduino.h>

struct L293DConfig {
    int leftDir;
    int leftPWM;
    int rightDir;
    int rightPWM;
    int maxPWM = 255;
};

class L293D {
public:
    L293D();

    void begin(L293DConfig config);

    void setCoefficients(float left, float right);
    void saveCoefficients();
    bool loadCoefficients();

    void setLeftSpeed(int speed);
    void setRightSpeed(int speed);

    void forward(int speed);
    void backward(int speed);
    void turnLeft(int speed);
    void turnRight(int speed);
    void stop();

    void drive(const String& direction, int speed);

private:
    L293DConfig _cfg;
    float _leftCoef;
    float _rightCoef;
    bool _begun;

    void writeMotor(int dirPin, int pwmPin, int speed, float coef);
};
