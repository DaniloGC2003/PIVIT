/*
  ESP-NOW Demo - Receive
  esp-now-demo-rcv.ino
  Reads data from Initiator
  
  DroneBot Workshop 2022
  https://dronebotworkshop.com

  modified by Danilo Gondo Chinen
*/

// Include Libraries
#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>
#include "../utils/utils.h"

#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
//for on-board RGB LED control
CRGB leds[1];

//variables to calculate elapsed time
unsigned long start_time = 0;
unsigned long end_time = 0;
bool chronometer_active = false;

//Used to send the ACK message
bool ack_pending = false;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
int ID_number = 0;

// payload for ESP-NOW messages
struct_message payload_ESPNOW;


void print_ESPNOW_payload(struct_message payload)
{
  Serial.println("payload's contents:");
  Serial.print("direction: ");
  Serial.println(payload.direction);
  Serial.print("code: ");
  Serial.println(payload.code);
  Serial.print("sender's ID: ");
  Serial.println(payload.sender_id);
  Serial.print("Reaction time: ");
  Serial.println(payload.reaction_time);
}

// Callback function executed when data is received
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingData, int len) {
  struct_message received_payload;
  memcpy(&received_payload, incomingData, sizeof(received_payload));
  Serial.println("Data received:");
  print_ESPNOW_payload(received_payload);

  if (received_payload.direction == DOWNLINK)
  {
    if ((received_payload.code == START_CHRONOMETER_SLAVE1 && ID_number == 1) || (received_payload.code == START_CHRONOMETER_SLAVE2 && ID_number == 2) ||
        (received_payload.code == START_CHRONOMETER_SLAVE3 && ID_number == 3) || (received_payload.code == START_CHRONOMETER_SLAVE4 && ID_number == 4) ||
        (received_payload.code == START_CHRONOMETER_SLAVE5 && ID_number == 5) || (received_payload.code == START_CHRONOMETER_SLAVE6 && ID_number == 6) ||
        (received_payload.code == START_CHRONOMETER_SLAVE7 && ID_number == 7))
    {
      Serial.println("start chronometer");

      //turn LED green
      digitalWrite(PIN_BLUE, HIGH);
      digitalWrite(PIN_RED, HIGH);
      digitalWrite(PIN_GREEN, LOW);
      leds[0] = CRGB::Green;
      FastLED.show();
      
      //start counting time
      chronometer_active = true;
      start_time = millis();
      ack_pending = true;

    }
  }
}

void setup() {
  // Set up Serial Monitor
  Serial.begin(115200);
  Serial.println("Slave device boot...");

  /* GPIO setup */
  //for on-board RGB LED
  FastLED.addLeds<LED_TYPE, PIN_LED, COLOR_ORDER>(leds, 1);
  FastLED.setBrightness(255);

  //RGB LED pins
  pinMode(PIN_BLUE, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  //Choronometer pin
  pinMode(BUTTON_PIN_CHRONOMETER, INPUT_PULLUP);  
  //ID pins
  pinMode(PIN_ID_0, INPUT_PULLUP);
  pinMode(PIN_ID_1, INPUT_PULLUP);
  pinMode(PIN_ID_2, INPUT_PULLUP);

  //disable RGB LED
  digitalWrite(PIN_BLUE, HIGH);
  digitalWrite(PIN_RED, HIGH);
  digitalWrite(PIN_GREEN, HIGH);

  //check ID pins
  int ID0 = digitalRead(PIN_ID_0);
  int ID1 = digitalRead(PIN_ID_1);
  int ID2 = digitalRead(PIN_ID_2);
  Serial.println(ID2);
  Serial.println(ID1);
  Serial.println(ID0);
  ID_number = ID2 * 4 + ID1 * 2 + ID0;
  Serial.print("I am slave number ");
  Serial.println(ID_number);

  /*ESP-NOW setup*/
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

  payload_ESPNOW.direction = UPLINK;
  payload_ESPNOW.code = 0;
  payload_ESPNOW.sender_id = ID_number;
  payload_ESPNOW.reaction_time = 0.0;


  Serial.println("setup completed");
}
 
void loop() {
  if (ack_pending) {
    //Send response time to master device
    esp_err_t result = ESP_FAIL;
    payload_ESPNOW.code = UPLINK_ACK;
    payload_ESPNOW.reaction_time = 0;
    print_ESPNOW_payload(payload_ESPNOW);
    result = esp_now_send(broadcastAddress, 
                          (uint8_t*) &payload_ESPNOW,
                          sizeof(payload_ESPNOW)); 
    if (result == ESP_OK) {
      Serial.println("Sent ACK");
      ack_pending = false;
    } else {
      Serial.println("Error on sending ACK");
    }
  }
  if (chronometer_active == true)
  {
    if (digitalRead(BUTTON_PIN_CHRONOMETER) == LOW)//if button was pressed while chronometer was active
    {
      Serial.println("BUTTON PRESSED. SUBMITTING RESPONSE TIME");

      //turn off LED
      digitalWrite(PIN_BLUE, HIGH);
      digitalWrite(PIN_RED, HIGH);
      digitalWrite(PIN_GREEN, HIGH);
      leds[0] = CRGB::Black;   // equivalent to (0,0,0)
      FastLED.show();
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
      payload_ESPNOW.reaction_time = time_delta;
      print_ESPNOW_payload(payload_ESPNOW);
      result = esp_now_send(broadcastAddress, 
                            (uint8_t*) &payload_ESPNOW,
                            sizeof(payload_ESPNOW)); 
      if (result == ESP_OK) {
        Serial.println("Sent reaction time");
      } else {
        Serial.println("Error on sending reaction time");
      }
      

    }
    delay(10);//10 ms delay to prevent watchdog from triggering.
  }
}