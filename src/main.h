#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define DUMP_AT_COMMANDS

#include <TinyGsmClient.h>
#include <Arduino.h>
// #include <gsm.h>
#include <Ticker.h>
#include <Preferences.h>
#include <RCSwitch.h>

#define NW_STATUS_LED 7
// #define SIGNAL_LED 7
#define ARMED_LED 6

#define UART_BAUD 9600
#define PIN_TX 16
#define PIN_RX 17
#define RST_PIN 15

#define RFPIN 15
#define SIRENPIN 5
#define BTN 0

// main.cpp
extern Preferences pref;
void arm();
void disarm();
void intrusion();
void panic();

// gsm_commands.cpp
void processPhoneCmd(String szFrom, String szCmd);

// state.cpp
extern bool bSendUnknownCode;
extern bool bArmed;
extern String szLastMsg;
void updateState(uint32_t rfdata);

// nwLedUpdate.cpp
void initNetworkLed();
void updateNetworkLed();