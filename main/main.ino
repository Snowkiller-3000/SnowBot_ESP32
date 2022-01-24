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
#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

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

// Network Constants
const char *ssid = "SnowKiller3000";
const char *password =  "123456789";
const int dns_port = 53;
const int http_port = 80;
const int ws_port = 1024;

// Electrical Constants
const float voltageCalVal = 0.007412109375; // ADC reading is multiplied by this value to get a voltage reading
const int speedLimit = 250; // Speed can be limited for safety during testing. Acceptable values are 0 to 255.
const int flipX = 1; // The X-value from the joystick can be sign flipped by making this -1
const int flipY = -1; // The Y-value from the joystick can be sign flipped by making this -1
const unsigned long motorTimeout = 30000; // If no commands received after this time, disable motors

// Global Variables
char msg_buf[10];
bool isConnected = 0;
unsigned long lastCommand = 0;
bool timedOut = 0;

// Create SPI object on HSPI pins (CLK 14, DI 13)
SPIClass SPI1(HSPI);

// Create Server
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(ws_port);

void setup() {
  SPI1.begin();

  Serial.begin(115200);
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

  disableMotors(); // Most important thing, disable motors!
  setMotor(1, 0); // Set both motors to zero
  setMotor(2, 0);

  // Make sure we can read the file system
  if ( !SPIFFS.begin()) {
    Serial.println("Error mounting SPIFFS");
    while (1);
  }

  // Start access point
  WiFi.softAP(ssid, password);

  // Print our IP address
  Serial.println();
  Serial.print("My IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println();

  // On HTTP request for root, provide index.html file
  server.on("/", HTTP_GET, onIndexRequest);

  // On HTTP request for style sheet, provide style.css
  server.on("/style.css", HTTP_GET, onCSSRequest);

  // Handle requests for pages that do not exist
  server.onNotFound(onPageNotFound);

  // Send joystick background
  server.on("/joystick-base.png", HTTP_GET, onJoystickBaseRequest);

  // Send joystick foreground
  server.on("/joystick-red.png", HTTP_GET, onJoystickRedRequest);

  // Start web server
  server.begin();

  // Start WebSocket server and assign callback
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
}

void loop() {
  webSocket.loop();
  if (!timedOut && ((millis() - lastCommand) > motorTimeout)) {
    disableMotors();
    timedOut = 1;
  }
}

float getBatteryVoltage() {
  return analogRead(battPin) * voltageCalVal;
}

void setMotor(int whichMotor, int newSpeed) { // speed value must be between -255 and 255

  if (newSpeed >= -speedLimit && newSpeed <= speedLimit) {

    int absSpeed = abs(newSpeed);

    Serial.print("Setting motor ");
    Serial.print(whichMotor);
    Serial.print(" to speed ");
    Serial.println(newSpeed);

    if (whichMotor == 1) { // Change Motor 1
      digiPotWrite(SPI_CS1, absSpeed);
      // if newSPeed == 0, don't change direction (prevent lurch)
      if (newSpeed > 0)
        digitalWrite(motorDir1, LOW);
      else if (newSpeed < 0)
        digitalWrite(motorDir1, HIGH);
    }
    else if (whichMotor == 2) { // Change Motor 2
      digiPotWrite(SPI_CS2, absSpeed);
      if (newSpeed > 0)
        digitalWrite(motorDir2, LOW);
      else if (newSpeed < 0)
        digitalWrite(motorDir2, HIGH);
    }
    else {
      Serial.println("Error: Invalid motor identifier");
    }
  }
  else {
    Serial.println("Error: Speed out of range");
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
  Serial.print("Setting actuator ");
  Serial.print(whichActuator);
  Serial.print(" to direction ");
  Serial.println(dir);

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
  bool validPort = 0;
  switch (auxPort) {
    case 1:
      validPort = 1;
      digitalWrite(aux1, state);
      break;
    case 2:
      validPort = 1;
      digitalWrite(aux2, state);
      break;
    case 3:
      validPort = 1;
      digitalWrite(aux3, state);
      break;
    default:
      Serial.println("Error: This aux port is reserved or does not exist");
  }
  if (validPort) {
    Serial.print("Setting port ");
    Serial.print(auxPort);
    Serial.print(" to state ");
    Serial.println(state);
  }
}

// Callback: receiving any WebSocket message
void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t * payload, size_t length) {

  lastCommand = millis();
  timedOut = 0;
  String str = (char *)payload;

  // Figure out the type of WebSocket event
  switch (type) {

    // Client has disconnected
    case WStype_DISCONNECTED:
      // Client disconnected, kill both motors
      setMotor(1, 0);
      setMotor(2, 0);
      disableMotors();
      Serial.println("Disconnected!");
      isConnected = 0;
      digitalWrite(ledPin, LOW);
      break;

    // New client has connected
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(client_num);
        Serial.printf("[%u] Connection from ", client_num);
        Serial.println(ip.toString());
        digitalWrite(ledPin, HIGH);
        isConnected = 1;
      }
      break;

    // Handle text messages from client
    case WStype_TEXT:

      Serial.print("New message: ");
      Serial.println(str);
      // Print out raw message
      //      Serial.printf("[%u] Received text: ", client_num);

      if (str.indexOf("M") != -1 ) {    // Motor joystick data sent
        float x = flipX * str.substring(1, str.indexOf(',')).toFloat();
        float y = flipY * str.substring(str.indexOf(',') + 1).toFloat();
        Serial.print("X: ");
        Serial.print(x);
        Serial.print(" Y: ");
        Serial.println(y);

        enableMotors();
        setMotor(1, mixMotor1(x, y));
        setMotor(2, mixMotor2(x, y));
      } else if (str.indexOf("P") != -1 ) {    // Plow joystick data sent
        float x = flipX * str.substring(1, str.indexOf(',')).toFloat();
        float y = flipY * str.substring(str.indexOf(',') + 1).toFloat();
        Serial.print("Plow X: ");
        Serial.print(x);
        Serial.print(" Plow Y: ");
        Serial.println(y);
        if (x > 0.25) {
          setActuator(0, 1);
        } else if (x < -0.25) {
          setActuator(0, -1);
        } else {
          setActuator(0, 0);
        }
        if (y > 0.25) {
          setActuator(1, 1);
        } else if (y < -0.25) {
          setActuator(1, -1);
        } else {
          setActuator(1, 0);
        }

      } else if (strcmp((char *)payload, "STOP_M") == 0) {    // Joystick released, stop drive motors
        Serial.println("Stopping Motors");
        setMotor(1, 0);
        setMotor(2, 0);
      } else if (strcmp((char *)payload, "STOP_P") == 0) {    // Joystick released, stop plow actuators
        Serial.println("Stopping Actuators");
        setActuator(0, 0);
        setActuator(1, 0);
      } else {
        Serial.println("Message not recognized");
      }
      break;

    // For everything else: do nothing
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    default:
      break;
  }
}

void enableMotors() {
  // Turn on the main power solenoid to the motor controllers
  if (isConnected) { // only if a client is connected
    digitalWrite(aux4, HIGH);
    Serial.println("Enabling motor controllers");
  }
  else {
    Serial.println("Error: No client connected");
  }
}

void disableMotors() {
  // Turn off the main power solenoid to the motor controllers
  digitalWrite(aux4, LOW);
  Serial.println("Disabling motor controllers");
}

void onIndexRequest(AsyncWebServerRequest * request) {
  // Callback: send homepage
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/index.html", "text/html");
}

void onCSSRequest(AsyncWebServerRequest * request) {
  // Callback: send style sheet
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/style.css", "text/css");
}

void onPageNotFound(AsyncWebServerRequest * request) {
  // Callback: send 404 if requested file does not exist
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
  request->send(404, "text/plain", "Error (404)\nYou've made a huge mistake!");
}

void onJoystickBaseRequest(AsyncWebServerRequest * request) {
  // Callback: send joystick-base.png
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/joystick-base.png", "image/png");
}

void onJoystickRedRequest(AsyncWebServerRequest * request) {
  // Callback: send joystick-red.png
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                 "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/joystick-red.png", "image/png");
}

int mixMotor1(float X, float Y) {
  int result = (int)((Y - X) * speedLimit);
  result = constrain(result, -speedLimit, speedLimit);
  return result;
}

int mixMotor2(float X, float Y) {
  int result = (int)((Y + X) * speedLimit);
  result = constrain(result, -speedLimit, speedLimit);
  return result;
}
