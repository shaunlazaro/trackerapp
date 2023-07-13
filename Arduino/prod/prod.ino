// Prereqs: GY521, AccelStepper
#include "BluetoothSerial.h"
#include <AccelStepper.h>
#include "GY521.h"

// Pin Numbers:
int in1 = 12; // Motor pins
int in2 = 14;
int in3 = 27;
int in4 = 26;
int ledg = 32; // Led pins
int ledr = 33;
int base = 5; // Controls transistor

// Accelerometer:
GY521 sensor(0x68);
float xTotal = 0;
float yTotal = 0;
int measureCount = 0;
int desiredAngle = 20;
bool desiredAngleInit = false;

// LED:
long blinkTimer = 0;
long BLINK_INTERVAL = 1000;
bool blinkToggle = true;

// Motor:
AccelStepper stepper; // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5
                      // AccelStepper stepper(AccelStepper::FULL4WIRE, IN1, IN3, IN2, IN4) -> strange input pin order...
long motorTimer = 0;
long MOTOR_INTERVAL = 2000;
long motorOnRatio = 10;
bool motorOn = true;

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

  //Motor:
  //stepper = AccelStepper(AccelStepper::FULL4WIRE, in1, in3, in2, in4);
  stepper = AccelStepper(AccelStepper::FULL4WIRE, 12, 27, 14, 26);
  stepper.setMaxSpeed(100);
  stepper.setAcceleration(20);

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
  // Bluetooth Reading:
  if(ESPbt.available())
  {
    delay(100);
    int msgLength = ESPbt.available();
    char msgString[32];
    for(int i = 0; i < msgLength; i++)
    {
      msgString[i] = (char)ESPbt.read();
    }
    msgString[msgLength] = '\0'; // Append a null
    Serial.print("Recieved BT Message:");
    Serial.println(msgString);

    // Send "SET" from bluetooth to tell motor that the panel is flat.
    if(strcmp(msgString, "SET") == 0)
    {
      Serial.print("Setting current position: ");
      Serial.print(stepper.currentPosition());
      Serial.println(" to position 0.");
      stepper.setCurrentPosition(0);
    }

    // Reading desired angle
    if(!desiredAngleInit)
    {
      desiredAngle = atof(msgString);
      desiredAngleInit = true;
      Serial.print("Desired Angle Has Been Uploaded: ");
      Serial.println(desiredAngle);
    }
    else
    {
      float motorToPos = atof(msgString);
      stepper.move(motorToPos);
      Serial.print("Moving relative distance: ");
      Serial.println(motorToPos);
    }
  }

  // Accelerometer Read:
  sensor.read();
  float x = sensor.getAngleX(); // Tilt we care about
  float y = sensor.getAngleY(); // Indicates non-level surface
  //float z = sensor.getAngleZ(); // Not useful.
  
  xTotal += x;
  yTotal += y;
  measureCount++;

  // Bluetooth Reporting: Send Current Orientation
  // Change LED on report.
  if (millis() - readTimer >= READ_INTERVAL){
    if (!cong){
      // Include message to send here like so:
      ESPbt.print("Tilt: ");
      ESPbt.print(x);
      ESPbt.print("Skew: ");
      ESPbt.print(y);
      ESPbt.print("Time: ");
      ESPbt.println(String(millis()));
      
      Serial.println(stepper.currentPosition());
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


  // Motor:
  if(motorOn)
  {
    stepper.run();
    if ((millis() - motorTimer) / motorOnRatio >= MOTOR_INTERVAL){
      motorOn = false;
      motorTimer = millis();
      stepper.disableOutputs();
    }
  }
  else
  {
    if (millis() - motorTimer >= MOTOR_INTERVAL){
      motorOn = true;
      motorTimer = millis();
      stepper.enableOutputs();
    }
  }

  if(Serial.available())
  {
    int msgLength = Serial.available();
    char msgString[32];
    for(int i = 0; i < msgLength; i++)
    {
      msgString[i] = (char)Serial.read();
    }
    msgString[msgLength] = '\0'; // Append a null
    float motorTo = atof(msgString);
    Serial.print("Moving to: ");
    Serial.println(motorTo);
    stepper.moveTo(motorTo);
  }
}


float posFromAngle(float degrees)
{

}