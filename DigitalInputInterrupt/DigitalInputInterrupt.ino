#include "Wire.h"
#include <MPU6050_light.h>

#define BAUDRATE 115200  //Sets max baud rate speed
#define GPIO2 2

MPU6050 mpu(Wire);
unsigned long timer = 0;

struct messageFormat {
  float pitch;
  float roll;
  float yaw;
  int gpio2;
};
messageFormat comMessage;

void setup() {
  Serial.begin(BAUDRATE);
  setupGPIO();
  setupMPU();

}

void setupGPIO() {
  pinMode(GPIO2, INPUT); // Sets PIN to input
  digitalWrite(GPIO2, HIGH); // Sets PIN to have pull up
  attachInterrupt (digitalPinToInterrupt(GPIO2), interruptGPIO2, FALLING);
}

void setupMPU() {
  Wire.begin();
  byte status = mpu.begin();
  while (status != 0) { Serial.println("MPU SETUP ERROR"); } // stop everything if could not connect to MPU6050
  mpu.calcOffsets(); // gyro and accelero
}

void interruptGPIO2() {
  comMessage.gpio2 = 1;
}

void readAngles() {
  mpu.update();
  if ((millis() - timer) > 10) { // update data every 10ms
    comMessage.pitch = mpu.getAngleX();
    comMessage.roll = mpu.getAngleY();
    comMessage.yaw = mpu.getAngleZ();
    timer = millis();
  }
}
void clearOldData() {
  comMessage.gpio2 = 0;
}

void sendMessage() {
  Serial.print("R,");
  Serial.print(comMessage.pitch,0);
  Serial.print(",");
  Serial.print(comMessage.roll,0);
  Serial.print(",");
  Serial.print(comMessage.yaw,0);
  Serial.print(",");
  Serial.print(comMessage.gpio2);
  Serial.print(",");
  Serial.println("");
  clearOldData();
}

void loop() {
  int input = Serial.read();
  if (input == 1) {
    sendMessage();
  }
  readAngles();
}
