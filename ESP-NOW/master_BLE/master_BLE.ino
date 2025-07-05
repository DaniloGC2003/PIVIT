/*
  Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
  Ported to Arduino ESP32 by Evandro Copercini
  updated by chegewara and MoThunderz

  modified by Danilo Gondo Chinen
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_now.h>
#include <WiFi.h>
#include "../utils/utils.h"

//BLE characteristic 1: sends response time
//BLE characteristic 2: sends and receives signals
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic_2 = NULL;
BLECharacteristic* pCharacteristic_client_comm = NULL;
BLEDescriptor *pDescr;
BLEDescriptor *pDescr_2;
BLE2902 *pBLE2902;
BLE2902 *pBLE2902_2;

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

// payload for ESP-NOW messages
struct_message payload_ESPNOW;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR1_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26a8"//Characteristic to send response time and device's ID
#define CHAR2_UUID          "2f6d0e14-ea50-4891-99d8-4bfbb72c1625"//Characteristic to communicate with clients (app and ESPs)

void printBufferBytes(uint8_t *buffer, size_t length) {
  Serial.println("---- Buffer Dump ----");
  for (size_t i = 0; i < length; i++) {
    Serial.print("Byte ");
    Serial.print(i);
    Serial.print(": 0x");
    if (buffer[i] < 0x10) Serial.print('0');  // leading zero for single-digit hex
    Serial.print(buffer[i], HEX);
    Serial.print(" (");
    Serial.print(buffer[i], DEC);
    Serial.println(")");
  }
  Serial.println("----------------------");
}

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


void sendMessageBLE(int sender_id, unsigned long time_delta)
{
  byte_arr[0] = sender_id;
  byte_arr[1] = (uint8_t)(time_delta & 0xFF);
  byte_arr[2] = (uint8_t)((time_delta >> 8) & 0xFF);
  byte_arr[3] = (uint8_t)((time_delta >> 16) & 0xFF);
  byte_arr[4] = (uint8_t)((time_delta >> 24) & 0xFF);

  Serial.println("bytes to be sent to app: ");
  for (int i = 0; i < 5; i++) {
    Serial.println(byte_arr[i], DEC);  // Print each byte as decimal
  }
  
  //Send response time to app client
  pCharacteristic->setValue(byte_arr, sizeof(byte_arr));
  pCharacteristic->notify();
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

//BLE server callback functions
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("on connect");
      // do stuff here on connecting
      Serial.println("Device connected");

      //pServer->stopAdvertising();//get ready for other clients to connect
      oldDeviceConnected = deviceConnected;


      //Visual signal for user to know connection was successful
      digitalWrite(PIN_BLUE, LOW);
      digitalWrite(PIN_RED, LOW);
      digitalWrite(PIN_GREEN, LOW);
      delay(1000);
      digitalWrite(PIN_BLUE, HIGH);
      digitalWrite(PIN_RED, HIGH);
      digitalWrite(PIN_GREEN, HIGH);
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE device disconnected.");
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("Device disconnected. Start advertising");
      oldDeviceConnected = deviceConnected;

      //Visual signal for user to know device was disconnected
      digitalWrite(PIN_BLUE, HIGH);
      digitalWrite(PIN_RED, LOW);
      digitalWrite(PIN_GREEN, HIGH);
      delay(1000);
      digitalWrite(PIN_BLUE, HIGH);
      digitalWrite(PIN_RED, HIGH);
      digitalWrite(PIN_GREEN, HIGH);
    }
};

//callback function for BLE characteristic 2
class CharacteristicCallBack: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) override { 
    String pChar2_value_string = pChar->getValue();                // This part is updated slightly from the video
    int pChar2_value_int = pChar2_value_string.toInt();
    Serial.println("pChar2: " + String(pChar2_value_int)); 

    if (pChar2_value_int == START_CHRONOMETER_MASTER)
    {
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
          //send BLE data
          sendMessageBLE(master_id, time_delta);

        }
        delay(10);//10 ms delay to prevent watchdog from triggering.
      }
    }
    else if (pChar2_value_int == START_CHRONOMETER_SLAVE1)
    {
      Serial.println("Start slave 1. Sending serial data");
      sendMessageSerial(DOWNLINK, START_CHRONOMETER_SLAVE1, MASTER_ID, 0);
    }
    else if (pChar2_value_int == START_CHRONOMETER_SLAVE2)
    {
      Serial.println("Start slave 2. Sending serial data");
      sendMessageSerial(DOWNLINK, START_CHRONOMETER_SLAVE2, MASTER_ID, 0);
    }
    else if (pChar2_value_int == START_CHRONOMETER_SLAVE3)
    {
      Serial.println("Start slave 3. Sending serial data");
      sendMessageSerial(DOWNLINK, START_CHRONOMETER_SLAVE3, MASTER_ID, 0);
    }else if (pChar2_value_int == START_CHRONOMETER_SLAVE4)
    {
      Serial.println("Start slave 4. Sending serial data");
      sendMessageSerial(DOWNLINK, START_CHRONOMETER_SLAVE4, MASTER_ID, 0);
    }else if (pChar2_value_int == START_CHRONOMETER_SLAVE5)
    {
      Serial.println("Start slave 5. Sending serial data");
      sendMessageSerial(DOWNLINK, START_CHRONOMETER_SLAVE5, MASTER_ID, 0);
    }else if (pChar2_value_int == START_CHRONOMETER_SLAVE6)
    {
      Serial.println("Start slave 6. Sending serial data");
      sendMessageSerial(DOWNLINK, START_CHRONOMETER_SLAVE6, MASTER_ID, 0);
    }else if (pChar2_value_int == START_CHRONOMETER_SLAVE7)
    {
      Serial.println("Start slave 7. Sending serial data");
      sendMessageSerial(DOWNLINK, START_CHRONOMETER_SLAVE7, MASTER_ID, 0);
    }
  }
};

void setup() {
  Serial.begin(115200);  // Serial monitor
  Serial.println("Device boot...");

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);  // UART2
  Serial.println("UART Two-Way Chat started...");

  /* BLE setup */

  // Create the BLE Device
  BLEDevice::init("ESP32master");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics
  pCharacteristic = pService->createCharacteristic(
                      CHAR1_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );                   

  pCharacteristic_2 = pService->createCharacteristic(
                      CHAR2_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );  

  // Create BLE descriptors
  pDescr = new BLEDescriptor((uint16_t)0x2901);
  pDescr->setValue("A very interesting variable receba");
  pCharacteristic->addDescriptor(pDescr);
  pDescr_2 = new BLEDescriptor((uint16_t)0x2901);
  pDescr_2->setValue("description 2");
  pCharacteristic_2->addDescriptor(pDescr_2);

  
  //set notifications
  pBLE2902 = new BLE2902();
  pBLE2902->setNotifications(true);
  pCharacteristic->addDescriptor(pBLE2902);

  pBLE2902_2 = new BLE2902();
  pBLE2902_2->setNotifications(true);
  pCharacteristic_2->addDescriptor(pBLE2902_2);
  
  // Set callback function
  pCharacteristic_2->setCallbacks(new CharacteristicCallBack());
  
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting for a client connection to notify...");


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
    //Serial.print("bytes available before: ");
    //Serial.println(Serial2.available());
    char c = Serial2.read();
    //Serial.print("char read: ");
    //Serial.print(c);
    //Serial.println(int(c));

    // Match header bytes
    if (c == SERIAL_HEADER[headerIndex]) {
      headerIndex++;
      Serial.println("Char header match");
      if (headerIndex == HEADER_LEN) {
        Serial.println("Header match");
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

          if (payload_ESPNOW.code == UPLINK_REACTION_TIME)
          {
            sendMessageBLE(payload_ESPNOW.sender_id, payload_ESPNOW.reaction_time);
          }
        }
      }
    } else {
      headerIndex = 0;
    }
  }
}
