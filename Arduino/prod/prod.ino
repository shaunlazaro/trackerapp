#include "BluetoothSerial.h"
#include <AccelStepper.h>
#include "GY521.h"
#include <Adafruit_INA219.h>
#include <math.h>

// CONFIG:
int currentYear = 2023;
int currentMonth = 7;
int currentDay = 17;

// Multimeter:
Adafruit_INA219 ina219;
float current_mA = 0;
float voltage = 0;
float power_mW = 0;

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
long MOTOR_INTERVAL = 60000;

// BT:
BluetoothSerial ESPbt;
long readTimer = 0;
long READ_INTERVAL = 1500;
bool cong = false; 

// Timing:
double R_list[288];
bool settingTime = false;
int millisBaseline = 0;
int minsBaseline = 0;

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

//
// Helper functions
//
// Port of sun.py:
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962
#endif

double deg2rad(double deg) {
  return deg * M_PI / 180.0;
}

double rad2deg(double rad) {
  return rad * 180.0 / M_PI;
}

double sunPosition(int year, int month, int day, double hour, double m, double s, double lat, double longitude, double &az, double &el) {

  double twopi = 2.0 * M_PI;

  // Get day of the year, e.g. Feb 1 = 32, Mar 1 = 61 on leap years
  int len_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30};
  day = day + len_month[month-1];
  int leapdays = (year % 4 == 0 && (year % 400 == 0 || year % 100 != 0) && day >= 60 && !(month == 2 && day == 60));
  day += leapdays;

  // Get Julian date - 2400000
//  double jd = 32916.5 + (year - 1949) * 365 + floor((year - 1949) / 4) + day + (hour + (double) m / 60.0 + s / 3600.0) / 24.0;
  double jd = 32916.5 + (year - 1949) * 365 + floor((year - 1949) / 4) + day + (hour + static_cast<double>(m) / 60.0 + s / 3600.0) / 24.0;
  // The input to the Astronomer's almanac is the difference between
  // the Julian date and JD 2451545.0 (noon, 1 January 2000)
  double time = jd - 51545.0;

  // Ecliptic coordinates

  // Mean longitude
  double mnlong = 280.460 + 0.9856474 * time;
  mnlong = fmod(mnlong, 360.0);
  if (mnlong < 0) mnlong += 360.0;

  // Mean anomaly
  double mnanom = 357.528 + 0.9856003 * time;
  mnanom = fmod(mnanom, 360.0);
  if (mnanom < 0) mnanom += 360.0;
  mnanom *= deg2rad(1.0);

  // Ecliptic longitude and obliquity of the ecliptic
  double eclong = mnlong + 1.915 * sin(mnanom) + 0.020 * sin(2.0 * mnanom);
  eclong = fmod(eclong, 360.0);
  if (eclong < 0) eclong += 360.0;
  double oblqec = 23.439 - 0.0000004 * time;
  eclong = deg2rad(eclong);
  oblqec = deg2rad(oblqec);

  // Celestial coordinates
  // Right ascension and declination
  double num = cos(oblqec) * sin(eclong);
  double den = cos(eclong);
  double ra = atan(num / den);
  if (den < 0) ra += M_PI;
  if (den >= 0 && num < 0) ra += twopi;
  double dec = asin(sin(oblqec) * sin(eclong));

  // Local coordinates
  // Greenwich mean sidereal time
  double gmst = 6.697375 + 0.0657098242 * time + hour;
  gmst = fmod(gmst, 24.0);
  if (gmst < 0) gmst += 24.0;

  // Local mean sidereal time
  double lmst = gmst + longitude / 15.0;
  lmst = fmod(lmst, 24.0);
  if (lmst < 0) lmst += 24.0;
  lmst = deg2rad(lmst * 15.0);

  // Hour angle
  double ha = lmst - ra;
  if (ha < -M_PI) ha += twopi;
  if (ha > M_PI) ha -= twopi;

  // Latitude to radians
  lat = deg2rad(lat);

  // Azimuth and elevation
  el = asin(sin(dec) * sin(lat) + cos(dec) * cos(lat) * cos(ha));
  az = asin(-cos(dec) * sin(ha) / cos(el));

  // For logic and names, see Spencer, J.W. 1989. Solar Energy. 42(4):353
  bool cosAzPos = (0 <= sin(dec) - sin(el) * sin(lat));
  bool sinAzNeg = (sin(az) < 0);
  az += cosAzPos && sinAzNeg ? twopi : 0;
  if (!cosAzPos) {
    az = M_PI - az;
  }

  el = rad2deg(el);
  az = rad2deg(az);
  lat = rad2deg(lat);

  return az, el;
}

double R_opt(double beta_ax, double az_ax, double el, double az, double limit = 90.0) {
  beta_ax = deg2rad(beta_ax);
  az_ax = deg2rad(az_ax);

  double zen = M_PI_2 - deg2rad(el);

  double azrad = deg2rad(az);

  double arg = sin(zen) * sin(azrad - az_ax) /
               (sin(zen) * cos(azrad - az_ax) * sin(beta_ax) +
                cos(zen) * cos(beta_ax));

  double phi = 0.0;
  if ((arg < 0.0) && ((azrad - az_ax) > 0.0)) {
    phi = M_PI;
  } else if ((arg > 0.0) && ((azrad - az_ax) < 0.0)) {
    phi = -M_PI;
  }

  double R = rad2deg(atan(arg)) + rad2deg(phi);

  if (R > 90.0) {
    R = limit;
  } else if (R < -90.0) {
    R = -limit;
  }

  return R;
}

// 
// Program:
//

void setup()
{  
  Serial.begin(115200);

  // Initialize the INA219.
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  ina219.setCalibration_16V_400mA();

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
  ESPbt.register_callback(BT_EventHandler);

  // Pregenerate a list of angles for every 5 minutes.
  double beta_ax = 20.0;
  double az_ax = 180.0;
  double area = 25.0 / 10000.0;

  Serial.println("Begin Printing R:");
  // Calculate azimuth and elevation for every 5 minutes on June 1, 2023
  int totalMinutes = 0;
  int timezone = 4; // TZ is -4 Toronto...
  for (int i = 0; i < 288; i++) {
    double hr = totalMinutes / 60.0 + timezone;
    double mn = totalMinutes % 60;

    double az, el;
    sunPosition(currentYear, currentMonth, currentDay, hr, mn, 0, 43.5, -80.5, az, el); // Replace lat and long with your desired location
    R_list[i] = R_opt(beta_ax, az_ax, el, az);

    // Add 5 minutes to the total elapsed minutes for the next iteration
    totalMinutes += 5;
    
    Serial.print("Hours:");
    Serial.print(hr);
    Serial.print("Minutes:");
    Serial.print(mn);
    Serial.print("Index: ");
    Serial.print(i);
    Serial.print("  El: ");
    Serial.print(el);
    Serial.print("  Az: ");
    Serial.print(az);
    Serial.print("  R: ");
    Serial.println(R_list[i]);
  }
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

    String receivedMsg = String(msgString); // Convert to String
    // Trim leading and trailing white spaces from the received message
    receivedMsg.trim();
    // Convert the trimmed String back to a char array (C-style string)
    receivedMsg.toCharArray(msgString, 32);

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
    // Send "TIME" from BT to let the board know the next message will be the current time (in minutes).
    else if(strcmp(msgString, "TIME") == 0)
    {
      Serial.print("Now setting time:");
      settingTime = true;
    }
    // Next message after starting to set time:
    else if(settingTime)
    {
      int currentTimeMin = atof(msgString);
      settingTime = false;
      Serial.print("The current time has been set: ");
      Serial.println(currentTimeMin);
      minsBaseline = currentTimeMin;
      millisBaseline = millis();
    }
    // Reading desired angle first if nothing else needs to be read...
    else if(!desiredAngleInit)
    {
      desiredAngle = atof(msgString);
      desiredAngleInit = true;
      Serial.print("Desired Angle Has Been Uploaded: ");
      Serial.println(desiredAngle);
    }
    // Lastly we send signals 
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
  
  // Apply low pass filter (average) to remove noise.
  xTotal += x;
  yTotal += y;
  measureCount++;

  // Bluetooth Reporting: Send Current Orientation
  // Change LED on report.
  if (millis() - readTimer >= READ_INTERVAL){
    // Read Current/Voltage:
    digitalWrite(base, HIGH);
    delay(50);
    current_mA = ina219.getCurrent_mA();
    digitalWrite(base, LOW);
    delay(50);
    voltage = ina219.getBusVoltage_V();
    // Max power (mW) is at roughly 0.7x the voltage
    power_mW = (0.7*voltage)*current_mA;

    if (!cong){

      // Send setup related info until we set a time (consider that to be "beginning of trial"):
      String output = "";
      if(millisBaseline == 0)
      {
        output += "Tilt:";
        output += x;
        output += ":Ms:";
        output+= millis();
        output+= ":Current Position:";
        output+= stepper.currentPosition();
        ESPbt.println(output);
        digitalWrite(ledg, LOW);
      }
      else if((int)stepper.currentPosition() == (int)stepper.targetPosition())
      {        
        int currentMins = minsBaseline + (double)(millis()-millisBaseline) / 60000.0;
        output+=currentMins;
        Serial.print("Desired Position:");
        Serial.println(R_list[currentMins/5]);
        output+= ",";
        output+= stepper.currentPosition();
        output+= ",";
        output+= power_mW;
        output+= ",";
        output+= current_mA;
        ESPbt.println(output);
        digitalWrite(ledg, HIGH);
      }

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
  if((int)stepper.currentPosition() == (int)stepper.targetPosition())
  {
    stepper.disableOutputs();
  }

  stepper.run();
    
  if(millisBaseline != 0)
  {
    int currentMins = minsBaseline + (double)(millis()-millisBaseline) / 60000.0;    
    // Every motor interval, we switch between pos 0 and desiredPos.
    // Used to get data for "No Tracking" and "Tracking" case in (relatively) the same timespan
    if(millis() - motorTimer >= MOTOR_INTERVAL)
    {
      Serial.print("Motor interval hit - Setting target position to: ");
      stepper.enableOutputs();
      if(stepper.currentPosition() == 0)
      {
        Serial.println(R_list[currentMins / 5] / 90.0 * 500.0);
        stepper.moveTo((int)(R_list[currentMins / 5] / 90.0 * 500.0));
      }
      else
      {
        Serial.println(0);
        stepper.moveTo(0);
      }
      motorTimer = millis();
    }
  }

  // Motor move to absolute position when recieving local serial input. Debug only.
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

