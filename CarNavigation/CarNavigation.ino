#include "PID.h"
#include "mDriver.h"

#include <BluetoothSerial.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

const char *pin = "1234"; // Change this to more secure PIN.

String device_name = "ESP32-BT-Slave";

BluetoothSerial SerialBT;

// Motor pins
#define AIN1 22
#define BIN1 21
#define AIN2 1
#define BIN2 19
#define PWMA 23
#define PWMB 3

// IR pins
#define extremeLeft 34
#define centerLeft 35
#define center 32
#define centerRight 33
#define extremeRight 25

// Calibration and normal modes' pins
#define calibration 17
#define normal 16

// Indicators
#define calibrationLED 15
#define normalLED 13

// Correction factors for easily modifying motor configuration
// line up with function names like forward.  Value can be 1 or -1
const int correctionA = -1;
const int correctionB = 1;

// Initializing motors.  The library will allow you to initialize as many motors as you have memory for.
Motor leftMotor = Motor(AIN1, AIN2, PWMA, correctionA);
Motor rightMotor = Motor(BIN1, BIN2, PWMB, correctionB);

// Motor speeds
int lsp, rsp;
int lfspeed = 150; // standard speed can be modified later

// PID constants
float Kp = 0;
float Kd = 0;
float Ki = 0;
// set-point variable
int sp = 0;
PID carPID(Kp, Ki, Kd); // PID object

// color map values
int minValues[5], maxValues[5], threshold[5], sensors[5];

void setup()
{
  Serial.begin(115200);
  SerialBT.begin(device_name); //Bluetooth device name
  Serial.printf("The device with name \"%s\" is started.\nNow you can pair it with Bluetooth!\n", device_name.c_str());

  #ifdef USE_PIN
    SerialBT.setPin(pin);
    Serial.println("Using PIN");
  #endif
  
  carPID.setSpeeds(lfspeed);
  pinMode(calibration, INPUT);
  pinMode(normal, INPUT);
  pinMode(calibrationLED, OUTPUT);
  pinMode(normalLED, OUTPUT);
  digitalWrite(normalLED, LOW);
  sensors[0] = extremeLeft;
  sensors[1] = centerLeft;
  sensors[2] = center;
  sensors[3] = centerRight;
  sensors[4] = extremeRight;

  for(int i = 0; i < 5; i++)
  {
    pinMode(sensors[i], INPUT);
  }
}


void loop()
{
  // waiting for calibration
  
  while (!digitalRead(calibration)) {}
  digitalWrite(calibrationLED, HIGH);
  delay(1000);
  carPID.calibrate(leftMotor, rightMotor, minValues, maxValues, threshold, sensors); // calibration mode
  digitalWrite(calibrationLED, LOW);
  Serial.println("main");
  for ( int i = 0; i < 5; i++)
  {
    threshold[i] = (minValues[i] + maxValues[i]) / 2;
    Serial.print(threshold[i]);
    Serial.print("   ");
  }
  Serial.println();
  Serial.println("max");
  for ( int i = 0; i < 5; i++)
  {
    Serial.print(maxValues[i]);
    Serial.print("   ");
  }
  Serial.println();
  Serial.println("min");
  for ( int i = 0; i < 5; i++)
  {
    threshold[i] = (minValues[i] + maxValues[i]) / 2;
    Serial.print(minValues[i]);
    Serial.print("   ");
  }
  Serial.println();
  // waiting to be set to normal mode
  while (!digitalRead(normal)) {}
  digitalWrite(normalLED, HIGH);
  delay(1000);

  // Normal mode in action
  while (1)
  {
    Serial.println("loop");
  for ( int i = 0; i < 5; i++)
  {
    threshold[i] = (minValues[i] + maxValues[i]) / 2;
    Serial.print(analogRead(sensors[i]));
    Serial.print("   ");
  }
  Serial.println();
    // Extreme left turn when extremeLeft sensor detects dark region while extremeRight sensor detects white region
    if (analogRead(extremeLeft) > threshold[0] && analogRead(extremeRight) < threshold[4] )
    {
//      lsp = 0; rsp = lfspeed;
//      leftMotor.drive(0);
//      rightMotor.drive(lfspeed);
      SerialBT.println("left");
      left(leftMotor, rightMotor, lfspeed);
    }

    // Extreme right turn when extremeRight sensor detects dark region while extremeLeft sensor detects white region
    else if (analogRead(extremeRight) > threshold[4] && analogRead(extremeLeft) < threshold[0])
    { 
//      lsp = lfspeed; rsp = 0;
//      leftMotor.drive(lfspeed);
//      rightMotor.drive(0);
      SerialBT.println("right");
      right(leftMotor, rightMotor, lfspeed);
    }
    else if (analogRead(center) > threshold[2])
    {
      // arbitrary PID constans will be tuned later
      Kp = 0.022 * (4000 - analogRead(center));
      Kd = 10 * Kp;
      //Ki = 0.0001;
      carPID.setConstants(Kp, Ki, Kd);
      sp = (analogRead(centerLeft) - analogRead(centerRight));
      carPID.setSetpoint(sp);
      carPID.linefollow(leftMotor, rightMotor, lsp, rsp);
    }
  }
}