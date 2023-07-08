#include "BluetoothSerial.h"

BluetoothSerial ESPbt;

long start = 0;

// Bluetooth Event Handler CallBack Function Definition
// If we don't pay attention to congestion (cong) 
// the buffer will fill and messages will stop being sent.
// The BluetoothSerial will also start blocking code execution 
// for a certain time interval when the buffer is full. 

bool cong = false; 

void BT_EventHandler(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_START_EVT) {
    Serial.println("Initialized SPP");
  }
  else if (event == ESP_SPP_SRV_OPEN_EVT ) {
    Serial.println("Client connected");
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

  start = millis();

  // IMPORTANT: Choose a unique name for your bluetooth module so you 
  //            don't connect to another group 
  ESPbt.begin("ESP32_name");


  Serial.println("Waiting for Start Signal...");

  // OPTIONAL: This waits for a message to be received from the computer 
  //           before continuing. 
  while(!ESPbt.available()) 
    delay(50);
  if (ESPbt.available()){  
    Serial.write(ESPbt.read());  
    Serial.print('\n');
  }  

  // register the callbacks defined above (most important: congestion)
  ESPbt.register_callback(BT_EventHandler);
}

void loop()
{

  // This conditional will wait for 100 ms to pass before 
  // reporting to the computer 
  if (millis() - start >= 100){
    if (!cong){
      // Include message to send here like so:
      ESPbt.println(String(millis()));
    }
    start = millis();
  }

}