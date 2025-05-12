#ifndef UTILS_H
#define UTILS_H

// payload struct used by ESP-NOW interfaces
typedef struct struct_message
{
  // Direction: downlink: "d"; uplink: "u".
  char direction;
  int code;
  char string[32];
} struct_message;

// Characters for identifying payload direction
#define UPLINK 'u'
#define DOWNLINK 'd'

// GPIO pins for RGB LED control
#define PIN_BLUE 4
#define PIN_RED 5
#define PIN_GREEN 6

// GPIO pin for push button
#define BUTTON_PIN_CHRONOMETER 0

// codes for BLE and ESP-NOW
#define START_CHRONOMETER_MASTER 11
#define START_CHRONOMETER_SLAVE 12
#define UPLINK_REACTION_TIME 13

// void print_ESPNOW_payload(struct_message payload);

#endif
