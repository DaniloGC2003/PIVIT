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

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// payload for ESP-NOW messages
struct_message payload_ESPNOW;


// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR1_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26a8"//Characteristic to send response time and device's ID
#define CHAR2_UUID          "2f6d0e14-ea50-4891-99d8-4bfbb72c1625"//Characteristic to communicate with clients (app and ESPs)


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
          //create payload
          byte_arr[0] = master_id;
          byte_arr[1] = (uint8_t)(time_delta & 0xFF);
          byte_arr[2] = (uint8_t)((time_delta >> 8) & 0xFF);
          byte_arr[3] = (uint8_t)((time_delta >> 16) & 0xFF);
          byte_arr[4] = (uint8_t)((time_delta >> 24) & 0xFF);
          
          //Send response time to app client
          pCharacteristic->setValue(byte_arr, sizeof(byte_arr));
          pCharacteristic->notify();

        }
        delay(10);//10 ms delay to prevent watchdog from triggering.
      }
    }
    else if (pChar2_value_int == START_CHRONOMETER_SLAVE)
    {
      esp_err_t result = ESP_FAIL;
      payload_ESPNOW.code = START_CHRONOMETER_SLAVE;
      payload_ESPNOW.direction = DOWNLINK;
      payload_ESPNOW.reaction_time = 0.0;
      
      result = esp_now_send(broadcastAddress, 
                            (uint8_t*) &payload_ESPNOW,
                            sizeof(payload_ESPNOW)); 
      if (result == ESP_OK) {
        Serial.println("Sent");
      } else {
        Serial.println("Error on sending");
      }
    }
  }
};

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
  
  //Send response time to app client
  pCharacteristic->setValue(byte_arr, sizeof(byte_arr));
  pCharacteristic->notify();
}

void setup() {

  Serial.begin(115200);
  Serial.println("Device boot");

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
  
  // After defining the descriptors, set the callback functions
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


  delay(1000); // Espera 1 segundos
}