#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

unsigned long last = 0;

int pwm = 0; // CHANGE to actual pin used 
int base = 0; // CHANGE to actual pin used 
int duty = 0; // CHANGE to duty cycle to value between 0 and 255
int period = 100; // [ms] CHANGE to desired reporting period 

void setup(void) 
{
  Serial.begin(115200);
  while (!Serial) {
      // will pause until serial console opens
      delay(1);
  }
    
  Serial.println("Hello!");
  
  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  ina219.setCalibration_16V_400mA();

  pinMode(base, OUTPUT);
  pinMode(pwm, OUTPUT);

  analogWriteFrequency(100); // Change to desired PWM frequency

  digitalWrite(base,HIGH); //Set status of transistor BASE
  analogWrite(pwm,duty); // set duty cycle of PWM 

  Serial.println("Measuring voltage and current with INA219 ...");

  last = millis();
}

void loop(void) 
{
  // Execute this at specified time intervals 
  if (millis() - last >= period){
    float shuntvoltage = 0;
    float busvoltage = 0;
    float current_mA = 0;
    float loadvoltage = 0;
    float power_mW = 0;

    shuntvoltage = ina219.getShuntVoltage_mV();
    busvoltage = ina219.getBusVoltage_V();
    power_mW = ina219.getPower_mW();
    loadvoltage = busvoltage + (shuntvoltage / 1000);
    current_mA = ina219.getCurrent_mA();

    // CHOOSE WHAT INFO YOU WANT TO SEE, modify output as necessary
    // Each Serial.println() command ends the current message

    //Serial.print("Bus Voltage: "); Serial.print(busvoltage); Serial.println("V");
    //Serial.print("Shunt Voltage: "); Serial.print(shuntvoltage); Serial.println(" mV");
    Serial.print("Load Voltage:  "); Serial.print(loadvoltage); Serial.println(" V");
    Serial.print("Current:  "); Serial.print(current_mA); Serial.println(" mA");
    Serial.print("Power:         "); Serial.print(power_mW); Serial.println(" mW");
    Serial.println("");
    last = millis();
  }

}
