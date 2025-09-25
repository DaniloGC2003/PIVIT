#ifndef UTILS_H
#define UTILS_H

// payload struct used by ESP-NOW interfaces
typedef struct struct_message
{
  // Direction: downlink: "d"; uplink: "u".
  char direction;
  // code: type of information to be sent or received.
  int code;
  int sender_id;
  unsigned long reaction_time;
} struct_message;

#define RESEND_ATTEMPTS 2

#define MASTER_ID 0

// Characters for identifying payload direction
#define UPLINK 'u'
#define DOWNLINK 'd'

// GPIO pins for RGB LED control
#define PIN_BLUE 4
#define PIN_RED 5
#define PIN_GREEN 6

// GPIO pin for on-board LED
#define PIN_LED 48

// GPIO pin for push button
#define BUTTON_PIN_CHRONOMETER 0

// GPIO pins for identification
#define PIN_ID_2 7
#define PIN_ID_1 15
#define PIN_ID_0 16

//GPIO pins for serial communication
#define RXD2 16
#define TXD2 17

//GPIO pin for sending reset signal to ESP-NOW master
#define PIN_RESET 2

// codes for BLE, ESP-NOW and UART
#define UPLINK_REACTION_TIME 10 
#define START_CHRONOMETER_MASTER 11
#define START_CHRONOMETER_SLAVE1 12
#define START_CHRONOMETER_SLAVE2 13
#define START_CHRONOMETER_SLAVE3 14
#define START_CHRONOMETER_SLAVE4 15
#define START_CHRONOMETER_SLAVE5 16
#define START_CHRONOMETER_SLAVE6 17
#define START_CHRONOMETER_SLAVE7 18
#define RESET_ESP_NOW_MASTER 19

// serial data headers
#define SERIAL_HEADER "ESP"
#define HEADER_LEN 3
#define SERIAL_TRAILER "END"
#define SERIAL_START_CHRONOMETER "SSC"

// serial data info
#define PAYLOAD_LEN 13
#define PACKET_LEN (HEADER_LEN + PAYLOAD_LEN)

#endif
