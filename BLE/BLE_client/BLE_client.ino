/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara and MoThunderz
 */

#include "BLEDevice.h"
//#include "BLEScan.h"

#define START_CHRONOMETER 12

//GPIO pins
#define PIN_BLUE 4
#define PIN_RED 5
#define PIN_GREEN 6
#define BUTTON_PIN_CHRONOMETER 0

// Define UUIDs:
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID    charUUID_RESPONSE_TIME("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID    charUUID_COMM("2f6d0e14-ea50-4891-99d8-4bfbb72c1625");

// Some variables to keep track on device connected
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

// Define pointer for the BLE connection
static BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pRemoteChar_response;
BLERemoteCharacteristic* pRemoteChar_comm;

bool chronometer_active = false;
//variables to calculate elapsed time
unsigned long start_time = 0;
unsigned long end_time = 0;

// Callback function for Notify function
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                            uint8_t* pData,
                            size_t length,
                            bool isNotify) {
  Serial.println("callback notify");

  //if notification was sent through communication
  if(pBLERemoteCharacteristic->getUUID().toString() == charUUID_COMM.toString()) {

    // convert received bytes to integer
    uint32_t counter = pData[0];
    for(int i = 1; i<length; i++) {
      counter = counter | (pData[i] << i*8);
    }

    // print to Serial
    Serial.print("Characteristic (Notify) from server: ");
    Serial.println(counter );  

    if (counter == START_CHRONOMETER)
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
          //Send response time to app client
          //pCharacteristic->setValue(time_delta);
          //pCharacteristic->notify();

        }
        delay(10);//100 ms delay to prevent watchdog from triggering.
      }

    }
  }
}

// Callback function that is called whenever a client is connected or disconnected
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Connection to server estabilished.");
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("Disconnected from server.");
  }
};

// Function that is run whenever the server is connected
bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());
  
  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  if (pClient == nullptr)
  {
    Serial.println("NULL PCLIENT");
  }
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  Serial.println("after");
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  connected = true;
  pRemoteChar_response = pRemoteService->getCharacteristic(charUUID_RESPONSE_TIME);
  pRemoteChar_comm = pRemoteService->getCharacteristic(charUUID_COMM);
  if(connectCharacteristic(pRemoteService, pRemoteChar_response) == false)
    connected = false;
  else if(connectCharacteristic(pRemoteService, pRemoteChar_comm) == false)
    connected = false;

  if(connected == false) {
    pClient-> disconnect();
    Serial.println("At least one characteristic UUID not found");
    return false;
  }
  return true;
}

// Function to check Characteristic
bool connectCharacteristic(BLERemoteService* pRemoteService, BLERemoteCharacteristic* l_BLERemoteChar) {
  // Obtain a reference to the characteristic in the service of the remote BLE server.
  if (l_BLERemoteChar == nullptr) {
    Serial.print("Failed to find one of the characteristics");
    Serial.print(l_BLERemoteChar->getUUID().toString().c_str());
    return false;
  }
  Serial.println(" - Found characteristic: " + String(l_BLERemoteChar->getUUID().toString().c_str()));

  if(l_BLERemoteChar->canNotify())
    l_BLERemoteChar->registerForNotify(notifyCallback);

  return true;
}

// Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  //Called for each advertising BLE server.
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
  
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      //We have found our target server. We now need to connect to it. The connection will be handled by another function
      Serial.println("Found target server.");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
  
    } // Found our server
  }
}; 

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  
  //define pins for RGB LED
  pinMode(PIN_BLUE, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(BUTTON_PIN_CHRONOMETER, INPUT_PULLUP);  

  //disable RGB LED
  digitalWrite(PIN_BLUE, HIGH);
  digitalWrite(PIN_RED, HIGH);
  digitalWrite(PIN_GREEN, HIGH);

  if (!doConnect)
  {
    //We did not manage to found the BLE target server. There is no point in going further, since the client depends on the server to work.
    Serial.println("Target BLE server not found. Rebooting...");
    delay(2000);
    ESP.restart();
  }
  Serial.println("Setup completed successfully.");
} // End of setup.

void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");

      //Visual signal for user to know connection was successful
      digitalWrite(PIN_BLUE, LOW);
      digitalWrite(PIN_RED, LOW);
      digitalWrite(PIN_GREEN, LOW);
      delay(1000);
      digitalWrite(PIN_BLUE, HIGH);
      digitalWrite(PIN_RED, HIGH);
      digitalWrite(PIN_GREEN, HIGH);

    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  
  if (connected) {
  }
  else if(doScan)
  {
    //In case connection is lost, start scanning routine again.
    BLEDevice::getScan()->start(5);
    Serial.println("Starting scanning again.");

    //Visual signal for user to know device was disconnected
    digitalWrite(PIN_BLUE, HIGH);
    digitalWrite(PIN_RED, LOW);
    digitalWrite(PIN_GREEN, HIGH);
    delay(1000);
    digitalWrite(PIN_BLUE, HIGH);
    digitalWrite(PIN_RED, HIGH);
    digitalWrite(PIN_GREEN, HIGH);
  }
  delay(1000);
}