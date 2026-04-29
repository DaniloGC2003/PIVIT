#include <esp_now.h>
#include <WiFi.h>
#include "../utils/utils.h"



bool active_serial = true;

//array used to send data to the app
uint8_t byte_arr[5];

//master's ID
uint8_t master_id = MASTER_ID;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// payload for ESP-NOW messages
struct_message payload_ESPNOW;

//used to check if the ACK message was received
bool ack_received = true;
unsigned long start_chronometer_timestamp = 0;
short resend_attempts = 0;
#define ACK_TIME_TOLERANCE 2000
#define MAX_RESEND_ATTEMPTS 3

void sendMessageSerial(char direction, int code, int sender_id, unsigned long reaction_time) {
  uint8_t buffer[HEADER_LEN + 1 + 4 + 4 + 4];  // Total: 16 bytes
  int offset = 0;

  // Add header
  memcpy(buffer + offset, SERIAL_HEADER, HEADER_LEN);
  offset += HEADER_LEN;

  // Add data
  buffer[offset++] = direction;  // 1 byte

  memcpy(buffer + offset, &code, sizeof(code));
  offset += sizeof(code);

  memcpy(buffer + offset, &sender_id, sizeof(sender_id));
  offset += sizeof(sender_id);

  memcpy(buffer + offset, &reaction_time, sizeof(reaction_time));
  offset += sizeof(reaction_time);

  printBufferBytes(buffer, offset);
  // Send all at once
  Serial2.write(buffer, offset);
}


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

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery successful");
  } else {
    Serial.println("Delivery failed");
  }
}

// ESP-NOW callback function
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingData, int len) {
  active_serial = true;
  struct_message received_payload;
  memcpy(&received_payload, incomingData, sizeof(received_payload));
  Serial.println("received data from slave: ");
  print_ESPNOW_payload(received_payload);

  if (received_payload.code != UPLINK_ACK){
    //send serial data
    Serial.println("Sending serial data to BLE master");
    sendMessageSerial(received_payload.direction, received_payload.code, 
                      received_payload.sender_id, received_payload.reaction_time);  
  }
  else {
    Serial.println("ACK received");
    ack_received = true;
  }

}

void send_ESP_NOW_message ()
{
  esp_err_t result = ESP_FAIL;
  result = esp_now_send(broadcastAddress, 
                        (uint8_t*) &payload_ESPNOW,
                        sizeof(payload_ESPNOW)); 
  if (result == ESP_OK) {
    Serial.println("ESP-NOW message sent successfully");
  } else {
    Serial.println("Error on sending ESP-NOW message");
  }
}

void setup() {

  Serial.begin(115200);
  Serial.println("ESP-NOW master device boot...");

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


  // Register callback functions
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_register_send_cb(OnDataSent);


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

void printBufferBytes(uint8_t *buffer, size_t length) {
  Serial.println("---- Buffer Dump ----");
  for (size_t i = 0; i < length; i++) {
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": 0x");
    if (buffer[i] < 0x10) Serial.print('0');
    Serial.print(buffer[i], HEX);
    Serial.print(" (");
    Serial.print(buffer[i], DEC);
    Serial.println(")");
  }
  Serial.println("----------------------");
}

void loop() {
  static uint8_t headerIndex = 0;

  while (Serial2.available() && active_serial) {
    char c = Serial2.read();

    // Match header bytes
    if (c == SERIAL_HEADER[headerIndex]) {
      headerIndex++;
      //Serial.println("char header match");
      if (headerIndex == HEADER_LEN) {
        //Serial.println("header match");
        headerIndex = 0;

        // Wait until full payload is available
        delay(100);
        Serial.println("available bytes");
        Serial.println(Serial2.available());
        
        if (Serial2.available() >= 13) {
          uint8_t buffer[PAYLOAD_LEN];
          Serial2.readBytes(buffer, PAYLOAD_LEN);  // Read payload into buffer

          printBufferBytes(buffer, PAYLOAD_LEN);

          // Extract fields from buffer
          char direction = buffer[0];
          payload_ESPNOW.direction = direction;

          int code;
          memcpy(&code, buffer + 1, sizeof(code));
          payload_ESPNOW.code = code;

          int sender_id;
          memcpy(&sender_id, buffer + 5, sizeof(sender_id));
          payload_ESPNOW.sender_id = sender_id;

          unsigned long reaction_time;
          memcpy(&reaction_time, buffer + 9, sizeof(reaction_time));
          payload_ESPNOW.reaction_time = reaction_time;

          // Print parsed data
          Serial.println("✅ Valid packet received:");
          Serial.print("Direction: "); Serial.println(direction);
          Serial.print("Code: "); Serial.println(code);
          Serial.print("Sender ID: "); Serial.println(sender_id);
          Serial.print("Reaction Time: "); Serial.println(reaction_time);
          Serial.println("---------------------------");

          if (payload_ESPNOW.code == START_CHRONOMETER_SLAVE1 || payload_ESPNOW.code == START_CHRONOMETER_SLAVE2 ||
              payload_ESPNOW.code == START_CHRONOMETER_SLAVE3 || payload_ESPNOW.code == START_CHRONOMETER_SLAVE4 ||
              payload_ESPNOW.code == START_CHRONOMETER_SLAVE5 || payload_ESPNOW.code == START_CHRONOMETER_SLAVE6 ||
              payload_ESPNOW.code == START_CHRONOMETER_SLAVE7)
          { // Serial interface will only be reactivated once it receives an ESP_NOW uplink packet
            active_serial = false;// Stop ESP32 from listening to incoming serial data.
            send_ESP_NOW_message();
            ack_received = false;
            start_chronometer_timestamp = millis();
          }
        }
      }
    } else {
      headerIndex = 0;
    }
  }
    
  if (ack_received == false && (millis() - start_chronometer_timestamp) >= ACK_TIME_TOLERANCE) { //Message most likely not received by slave
    resend_attempts++;
    Serial.println("Chronometer activation message likely not received by slave. Trying again...");
    send_ESP_NOW_message(); // resend
    start_chronometer_timestamp = millis();
    if (resend_attempts == MAX_RESEND_ATTEMPTS) { //stop trying to send the message
      ack_received = true;
      String max_attempts_message = "MAX_RESEND_ATTEMPTS reached. Please check slave device ";
      max_attempts_message += payload_ESPNOW.code - 11;
      Serial.println(max_attempts_message); //sending the whole message in one string ensures it isn't interrupted by the message sending thread
    }
  }
}