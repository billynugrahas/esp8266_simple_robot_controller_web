#include <RobotWebUI.h>
#include <L293D.h>

const char* WIFI_SSID = "Niagraha";
const char* WIFI_PASS = "Niagraha2014";

RobotWebUI ui;
L293D motors;

void onMotor(const MotorCmd& cmd) {
    motors.drive(cmd.direction, cmd.speed);
}

void onCoef(const CoefCmd& cmd) {
    motors.setCoefficients(cmd.left, cmd.right);
    Serial.printf("[Coef] left=%.2f right=%.2f\n", cmd.left, cmd.right);
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n[Robot] Starting...");

    motors.begin({
        .leftDir   = D4,  // Motor A direction (GPIO 2)
        .leftPWM   = D2,  // Motor A speed     (GPIO 4)
        .rightDir  = D3,  // Motor B direction (GPIO 0)
        .rightPWM  = D1   // Motor B speed     (GPIO 5)
    });
    motors.setCoefficients(1.0, 1.0);

    ui.onMotorCommand(onMotor);
    ui.onCoefficientCommand(onCoef);
    ui.begin(WIFI_SSID, WIFI_PASS);
}

void loop() {
    ui.loop();
}
