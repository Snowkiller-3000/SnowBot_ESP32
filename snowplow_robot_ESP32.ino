/*
  VP/D36  Analog Input    Battery voltage sensing
  VN/D39  Analog Input    GPIO 6 (ADC only)
  D34     Analog Input    GPIO 5 (ADC only)
  D35     Analog Input    GPIO 4 (ADC only)
  D32     GPIO            GPIO 3 (ADC, Digital IO)
  D33     GPIO            GPIO 2 (ADC, Digital IO)
  D25     GPIO            GPIO 1 (ADC, DAC, Digital IO)
  D26     Digital Output  Motor direction #1
  D27     Digital Output  Motor direction #2
  D14     Digital Output  SPI Clock
  D12     -               Not in use
  D13     Digital Output  SPI Data
  D15     Digital Output  SPI Chip-select 1
  D2      Digital Output  On-board blue LED
  D4      Digital Output  SPI Chip-select 2
  RX2/D16 PWM Output      Actuator 2, Low
  TX2/D17 PWM Output      Actuator 2, High
  D5      PWM Output      Actuator 1, Low
  D18     PWM Output      Actuator 1, High
  D19     Digital Output  Auxiliary Power 1 (12V accessory)
  D21     Digital Output  Auxiliary Power 2 (12V accessory)
  RX0/D3  Digital Input   Serial RX, from RPi
  TX0/D1  Digital Output  Serial TX, to RPi
  D22     Digital Output  Auxiliary Power 3 (12V accessory)
  D23     Digital Output  Auxiliary Power 4 (12V accessory)
*/
#include "SPI.h"

#define ledPin  2
#define GPIO1Pin  25
#define GPIO2Pin  33
#define GPIO3Pin  32
#define GPIO4Pin  35
#define GPIO5Pin  34
#define GPIO6Pin  39

#define battPin   36

#define act2L 5
#define act2H 18
#define act1L 16
#define act1H 17

#define aux1  19
#define aux2  21
#define aux3  22
#define aux4  23

#define motorDir1 26
#define motorDir2 27

#define SPI_CLK 14
#define SPI_DI  13
#define SPI_CS1 15
#define SPI_CS2 4

SPIClass SPI1(HSPI);

float voltageCalVal = 0.007412109375; // ADC reading is multiplied by this value to get a voltage reading

void setup() {
  SPI1.begin();

  Serial.begin(9600);
  Serial.println("Hello");

  pinMode(SPI_CS1, OUTPUT);
  pinMode(SPI_CS2, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(motorDir1, OUTPUT);
  pinMode(motorDir2, OUTPUT);
  pinMode(act1L, OUTPUT);
  pinMode(act1H, OUTPUT);
  pinMode(act2L, OUTPUT);
  pinMode(act2H, OUTPUT);
  pinMode(aux1, OUTPUT);
  pinMode(aux2, OUTPUT);
  pinMode(aux3, OUTPUT);
  pinMode(aux4, OUTPUT);
}

void loop() {
  digitalWrite(ledPin, HIGH);

  for (int i = 0; i < 255; i++) {
    setMotor(0, i, 0);
    setMotor(1, i, 0);
    Serial.println(i);
    delay(25);
  }
  for (int i = 255; i > 0; i--) {
    setMotor(0, i, 0);
    setMotor(1, i, 0);
    Serial.println(i);
    delay(25);
  }
  digitalWrite(ledPin, LOW);
  for (int i = 0; i < 255; i++) {
    setMotor(0, i, 1);
    setMotor(1, i, 1);
    Serial.println(i);
    delay(25);
  }
  for (int i = 255; i > 0; i--) {
    setMotor(0, i, 1);
    setMotor(1, i, 1);
    Serial.println(i);
    delay(25);
  }

}

float getBatteryVoltage() {
  return analogRead(battPin) * voltageCalVal;
}

void setMotor(bool whichMotor, int newSpeed, bool dir) {

  if (!whichMotor) { // Change Motor 1
    digiPotWrite(SPI_CS1, newSpeed);
    if (dir)
      digitalWrite(motorDir1, LOW);
    else
      digitalWrite(motorDir1, HIGH);
  }
  else { // Change Motor 2
    digiPotWrite(SPI_CS2, newSpeed);
    if (dir)
      digitalWrite(motorDir2, LOW);
    else
      digitalWrite(motorDir2, HIGH);
  }
}

void digiPotWrite(int CS, int value)
{
  digitalWrite(CS, LOW);
  SPI1.transfer(0x11);
  SPI1.transfer(value);
  digitalWrite(CS, HIGH);
}

void setActuator(bool whichActuator, int dir) {
  if (whichActuator) {
    if (dir == 1) {
      digitalWrite(act1L, LOW);
      digitalWrite(act1H, HIGH);
    }
    else if (dir == -1) {
      digitalWrite(act1H, LOW);
      digitalWrite(act1L, HIGH);
    }
    else {
      digitalWrite(act1H, LOW);
      digitalWrite(act1L, LOW);
    }
  }
  else {
    if (dir == 1) {
      digitalWrite(act2L, LOW);
      digitalWrite(act2H, HIGH);
    }
    else if (dir == -1) {
      digitalWrite(act2H, LOW);
      digitalWrite(act2L, HIGH);
    }
    else {
      digitalWrite(act2H, LOW);
      digitalWrite(act2L, LOW);
    }
  }
}

void setAuxPwr(int auxPort, bool state) {
  switch (auxPort) {
    case 1:
      digitalWrite(aux1, state);
      break;
    case 2:
      digitalWrite(aux2, state);
      break;
    case 3:
      digitalWrite(aux3, state);
      break;
    case 4:
      digitalWrite(aux4, state);
      break;
  }
}
