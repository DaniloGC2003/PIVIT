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

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR1_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26a8"//Characteristic to send response time
#define CHAR2_UUID          "2f6d0e14-ea50-4891-99d8-4bfbb72c1625"//Characteristic to communicate with clients (app and ESPs)

//#define CHAR_CLIENT_COMM_UUID          "727edcfc-500c-4fcc-91ee-bb2f9d44584e"//Characteristic to communicate with client

//GPIO pins
#define PIN_BLUE 4
#define PIN_RED 5
#define PIN_GREEN 6
#define BUTTON_PIN_CHRONOMETER 0

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("on connect");
      // do stuff here on connecting
      Serial.println("Device connected");

      pServer->startAdvertising();//get ready for other clients to connect
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
      Serial.println("on disconnect");
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

//callback function for characteristic 2
class CharacteristicCallBack: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pChar) override { 
    String pChar2_value_string = pChar->getValue();                // This part is updated slightly from the video
    int pChar2_value_int = pChar2_value_string.toInt();
    Serial.println("pChar2: " + String(pChar2_value_int)); 

    if (pChar2_value_int == 11)
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
          Serial.print("Responde time: ");
          Serial.print(time_delta);
          Serial.println(" milliseconds.");
          //disable chronometer
          chronometer_active = false;
          //Send response time to app client
          pCharacteristic->setValue(time_delta);
          pCharacteristic->notify();

        }
        delay(100);//100 ms delay to prevent watchdog from triggering.
      }
    }
    else if (pChar2_value_int == 12)
    {
      pCharacteristic_2->notify();
    }
  }
};

void setup() {

  Serial.begin(115200);
  Serial.println("device boot");

  // Create the BLE Device
  BLEDevice::init("ESP32");

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
                      BLECharacteristic::PROPERTY_NOTIFY//ADD NOTIFY FUNCTIONALITIES
                    );  

  /*pCharacteristic_client_comm = pService->createCharacteristic(
                      CHAR_CLIENT_COMM_UUID
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  
                    );*/

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
  pBLE2902_2 = new BLE2902();
  pBLE2902_2->setNotifications(true);
  pCharacteristic->addDescriptor(pBLE2902);
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
  Serial.println("Waiting a client connection to notify...");


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
    // notify changed value
    if (deviceConnected) {


        //pCharacteristic->setValue(value);
        //pCharacteristic->notify();
        //value++;
        delay(1000);
    }
    // disconnecting
    /*if (!deviceConnected && oldDeviceConnected) {
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
    }*/
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        Serial.println("Device connected");

        pServer->startAdvertising();//get ready for other clients to connect
        oldDeviceConnected = deviceConnected;


        //Visual signal for user to know connection was successful
        digitalWrite(PIN_BLUE, LOW);
        digitalWrite(PIN_RED, LOW);
        digitalWrite(PIN_GREEN, LOW);
        delay(1000);
        digitalWrite(PIN_BLUE, HIGH);
        digitalWrite(PIN_RED, HIGH);
        digitalWrite(PIN_GREEN, HIGH);
    }
}