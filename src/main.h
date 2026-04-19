#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define DUMP_AT_COMMANDS

#include <TinyGsmClient.h>
#include <Arduino.h>
// #include <gsm.h>
#include <Ticker.h>
#include <Preferences.h>
#include <RCSwitch.h>

#define FW_VERSION "2.0.0"

#define NW_STATUS_LED 4
#define SIGNAL_LED 18
#define ARMED_LED 17

#define UART_BAUD 9600
#define PIN_TX 26
#define PIN_RX 33

#define RFPIN 14
#define SIRENPIN 21
#define BTN 0

#define SerialMon Serial
#define SerialAT Serial1

// main.cpp
extern Preferences pref;
void arm();
void disarm();
void intrusion();
void panic();

// gsm_commands.cpp
void initGsm();
void checkGsmSerial();

// state.cpp
extern bool bSendUnknownCode;
extern bool bArmed;
extern String szLastMsg;
void updateState(uint32_t rfdata);

// nwLedUpdate.cpp
void initNetworkLed();
void updateNetworkLed();

// ota.cpp
void setupOTA();