
#include <esp_now.h>
#include <WiFi.h>
#include "../utils/utils.h"

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

//variables to calculate elapsed time
unsigned long start_time = 0;
unsigned long end_time = 0;
bool chronometer_active = false;

//array used to send data to the app
uint8_t byte_arr[5];

//master's ID
uint8_t master_id = MASTER_ID;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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

// ESP-NOW callback function
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingData, int len) {
  Serial.println("oissss");
  struct_message received_payload;
  memcpy(&received_payload, incomingData, sizeof(received_payload));
  Serial.println("received data from slave: ");
  print_ESPNOW_payload(received_payload);

  byte_arr[0] = (uint8_t)(received_payload.sender_id & 0xFF);
  byte_arr[1] = (uint8_t)(received_payload.reaction_time & 0xFF);
  byte_arr[2] = (uint8_t)((received_payload.reaction_time >> 8) & 0xFF);
  byte_arr[3] = (uint8_t)((received_payload.reaction_time >> 16) & 0xFF);
  byte_arr[4] = (uint8_t)((received_payload.reaction_time >> 24) & 0xFF);
  String result = String((char*)byte_arr);

  Serial.println("bytes to be sent to BLE master: ");
  for (int i = 0; i < 5; i++) {
    Serial.println(byte_arr[i], DEC);  // Print each byte as decimal
  }

  //send serial data
  Serial.println("Sending serial data to BLE master");
  Serial2.println(SERIAL_HEADER + result);
}

void send_ESP_NOW_message ()
{
  esp_err_t result = ESP_FAIL;
  result = esp_now_send(broadcastAddress, 
                        (uint8_t*) &payload_ESPNOW,
                        sizeof(payload_ESPNOW)); 
  if (result == ESP_OK) {
    Serial.println("Sent");
  } else {
    Serial.println("Error on sending");
  }
}

void setup() {

  Serial.begin(115200);
  Serial.println("Device boot");

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);  // UART2
  Serial.println("UART Two-Way Chat started...");


  /* ESP-NOW setup */

  WiFi.mode(WIFI_STA);
  
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

  payload_ESPNOW.direction = DOWNLINK;
  payload_ESPNOW.code = 0;
  payload_ESPNOW.sender_id = MASTER_ID;
  payload_ESPNOW.reaction_time = 0.0;


  // Register callback function
  esp_now_register_recv_cb(OnDataRecv);


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
  static uint8_t headerIndex = 0;

  while (Serial2.available()) {
    char incoming = Serial2.read();

    // Header detection state machine
    if (incoming == SERIAL_HEADER[headerIndex]) {
      Serial.println("incoming");
      headerIndex++;
      if (headerIndex == HEADER_LEN) {
        Serial.println("full header matched");
        // Full header matched!
        headerIndex = 0;

        // Wait for struct data to arrive
        if (Serial2.available() >= sizeof(struct_message)) {
          struct_message msg;
          Serial2.readBytes((uint8_t *)&msg, sizeof(msg));

          Serial.println("Valid struct received:");
          Serial.print("Direction: "); Serial.println(msg.direction);
          Serial.print("Code: "); Serial.println(msg.code);
          Serial.print("Sender ID: "); Serial.println(msg.sender_id);
          Serial.print("Reaction Time: "); Serial.println(msg.reaction_time);
        }
      }
    } else {
      // Reset if mismatch
      Serial.println("mismatch");
      headerIndex = 0;
    }
  }

  //Incoming serial data
  /*while (Serial2.available()) {
    String incoming = Serial2.readStringUntil('\n');
    Serial.println("detected: " + incoming);
    if (incoming.startsWith(SERIAL_HEADER))
    {
      Serial.println("Received: " + incoming);
      int received_data = incoming.substring(3).toInt();
      if (received_data == START_CHRONOMETER_SLAVE)
      {
        Serial.println("Serial: start chronometer slave");
        payload_ESPNOW.code = START_CHRONOMETER_SLAVE;
        payload_ESPNOW.direction = DOWNLINK;
        payload_ESPNOW.reaction_time = 0.0;

        send_ESP_NOW_message();
      }

    }
  }*/

  //delay(1000); // Espera 1 segundos
}