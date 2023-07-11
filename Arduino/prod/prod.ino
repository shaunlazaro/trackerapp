#include "BluetoothSerial.h"
#include <AccelStepper.h>
#include "GY521.h"

// Pin Numbers:
int in1 = 12;
int in2 = 14;
int in3 = 27;
int in4 = 26;
int ledg = 33;
int ledr = 32;
int base = 5; // Controls transistor

// Accelerometer:
GY521 sensor(0x68);
float xTotal = 0;
float yTotal = 0;
int measureCount = 0;
int desiredAngle = 20;

// LED:
long blinkTimer = 0;
long BLINK_INTERVAL = 1000;
bool blinkToggle = true;

// Motor:
AccelStepper stepper; // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
                      // AccelStepper stepper(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4) -> strange input pin order...
long motorTimer = 0;
long MOTOR_INTERVAL = 20000;

// BT:
BluetoothSerial ESPbt;
long readTimer = 0;
long READ_INTERVAL = 1000;
bool cong = false; 

void BT_EventHandler(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_START_EVT) {
    Serial.println("Initialized SPP");
  }
  else if (event == ESP_SPP_SRV_OPEN_EVT ) {
    Serial.println("Client connected");
    cong = false;
  }
  else if (event == ESP_SPP_CLOSE_EVT  ) {
    Serial.println("Client disconnected");
  }
  else if (event == ESP_SPP_CONG_EVT){
    cong = true;
    Serial.println("Client not listening");
  }
}

void setup()
{  
  Serial.begin(115200);

  //Accelerometer:
  Wire.begin();
  delay(100);
  if (sensor.wakeup() == false)
  {
    Serial.println("Could not connect to GY521");
  }
  // Accel Calibration Values:
  sensor.axe = 0;
  sensor.aye = 0;
  sensor.aze = 0;
  sensor.gxe = 0;
  sensor.gye = 0;
  sensor.gze = 0;
  //  adjust when needed.
  sensor.setAccelSensitivity(0);  //  2g
  sensor.setGyroSensitivity(0);   //  250 degrees/s
  sensor.setThrottle(false);

  // LED:
  pinMode(ledg, OUTPUT);
  pinMode(ledr, OUTPUT);
  digitalWrite(ledg, LOW);
  digitalWrite(ledr, LOW);

  //Motor:
  stepper = AccelStepper(AccelStepper::FULL4WIRE, in1, in3, in2, in4);
  stepper.enableOutputs();
  stepper.setMaxSpeed(100);
  stepper.setAcceleration(20);
  stepper.moveTo(500);

  // Bluetooth:
  readTimer = millis();

  ESPbt.begin("solar_tracker_prototype"); // Hardcoded in client to this name.

  Serial.println("Waiting for Start Signal...");
  // OPTIONAL: This waits for a message to be received from the computer 
  //           before continuing. 
  // while(!ESPbt.available()) 
  //   delay(50);
  // if (ESPbt.available()){  
  //   Serial.write(ESPbt.read());  
  //   Serial.print('\n');
  // }  

  // register the callbacks defined above (most important: congestion)
  ESPbt.register_callback(BT_EventHandler);
}

void loop()
{
  if(ESPbt.available())
    Serial.print((char)ESPbt.read());

  // Accelerometer Read:
  sensor.read();
  float x = sensor.getAngleX(); // Tilt we care about
  float y = sensor.getAngleY(); // Indicates non-level surface
  float z = sensor.getAngleZ(); // Not useful.
  
  xTotal += x;
  yTotal += y;
  measureCount++;

  // Bluetooth Reporting: Send Current Orientation
  if (millis() - readTimer >= READ_INTERVAL){
    if (!cong){
      // Include message to send here like so:
      ESPbt.print("Tilt: ");
      ESPbt.print(x);
      ESPbt.print("Skew: ");
      ESPbt.print(y);
      ESPbt.print("Time: ");
      ESPbt.println(String(millis()));
      measureCount = 0;
      xTotal = 0;
      yTotal = 0;

      if(abs(x - desiredAngle) < 5)
      {
        digitalWrite(ledr, LOW);
      }
      else
      {
        digitalWrite(ledr, HIGH);
      }
    }
    readTimer = millis();
  }

  if (millis() - motorTimer >= MOTOR_INTERVAL){
    stepper.enableOutputs();
    stepper.moveTo(-stepper.currentPosition());
    delay(100);

    motorTimer = millis();
  }
  else if (stepper.distanceToGo() == 0)
  {
    //stepper.disableOutputs();
  }
}
