/*
  ESP-NOW Demo - Receive
  esp-now-demo-rcv.ino
  Reads data from Initiator
  
  DroneBot Workshop 2022
  https://dronebotworkshop.com
*/

// Include Libraries
#include <esp_now.h>
#include <WiFi.h>
#include "../utils/utils.h"

//variables to calculate elapsed time
unsigned long start_time = 0;
unsigned long end_time = 0;
bool chronometer_active = false;


uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// payload for ESP-NOW messages
struct_message payload_ESPNOW;





// Callback function executed when data is received
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingData, int len) {
  memcpy(&payload_ESPNOW, incomingData, sizeof(payload_ESPNOW));
  Serial.print("String Value: ");
  Serial.println(payload_ESPNOW.string);
  Serial.print("Direction: ");
  Serial.println(payload_ESPNOW.direction);

  if (payload_ESPNOW.direction == DOWNLINK)
  {
    if (payload_ESPNOW.code == START_CHRONOMETER_SLAVE)
      Serial.println("start chronometer");

      //turn LED green
      digitalWrite(PIN_BLUE, HIGH);
      digitalWrite(PIN_RED, HIGH);
      digitalWrite(PIN_GREEN, LOW);
      
      //start counting time
      chronometer_active = true;
      start_time = millis();

      while (chronometer_active == true)
      {
        if (digitalRead(BUTTON_PIN_CHRONOMETER) == LOW)//if button was pressed while chronometer was active
        {
          Serial.println("BUTTON PRESSED. SUBMITTING RESPONSE TIME");

          //turn off LED
          digitalWrite(PIN_BLUE, HIGH);
          digitalWrite(PIN_RED, HIGH);
          digitalWrite(PIN_GREEN, HIGH);
          end_time = millis();
          unsigned long time_delta = end_time - start_time;
          Serial.print("Response time: ");
          Serial.print(time_delta);
          Serial.println(" milliseconds.");
          //disable chronometer
          chronometer_active = false;

          //Send response time to master device
          esp_err_t result = ESP_FAIL;
          payload_ESPNOW.code = UPLINK_REACTION_TIME;
          result = esp_now_send(broadcastAddress, 
                                (uint8_t*) &payload_ESPNOW,
                                sizeof(payload_ESPNOW)); 
          if (result == ESP_OK) {
            Serial.println("Sent");
          } else {
            Serial.println("Error on sending");
          }
          

        }
        delay(10);//10 ms delay to prevent watchdog from triggering.
      }
  }
}

void setup() {
  // Set up Serial Monitor
  Serial.begin(115200);
  
  // Set ESP32 as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Configura o peer (opcional para broadcast)
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
  
  // Register callback function
  esp_now_register_recv_cb(OnDataRecv);

  payload_ESPNOW.direction = 'u';
  strcpy(payload_ESPNOW.string, "from slave");
  payload_ESPNOW.code = 0;


  /* GPIO setup */

  pinMode(PIN_BLUE, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(BUTTON_PIN_CHRONOMETER, INPUT_PULLUP);  

  //disable RGB LED
  digitalWrite(PIN_BLUE, HIGH);
  digitalWrite(PIN_RED, HIGH);
  digitalWrite(PIN_GREEN, HIGH);

  Serial.println("setup completed");
}
 
void loop() {

}