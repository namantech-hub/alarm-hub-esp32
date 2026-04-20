#define BHARATPI

#if defined(BHARATPI)
#define TINY_GSM_MODEM_SIM7600 1
#else 
#define TINY_GSM_MODEM_SIM800 1
#endif

#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define DUMP_AT_COMMANDS

#include <TinyGsmClient.h>
#include <Arduino.h>
// #include <gsm.h>
#include <Ticker.h>
#include <Preferences.h>
#include <RCSwitch.h>

#define FW_VERSION "2.0.0"

#if defined(BHARATPI)
#define NW_STATUS_LED 12
#define SIGNAL_LED 14
#define ARMED_LED 13

#define UART_BAUD 115200
#define PIN_TX 17
#define PIN_RX 16
#define PWR_PIN 32 // 4G module power pin, refer datasheet

#define RFPIN 5
#define SIRENPIN 4
#define BTN 0

#else

#define NW_STATUS_LED 4
#define SIGNAL_LED 18
#define ARMED_LED 17

#define UART_BAUD 9600
#define PIN_TX 26
#define PIN_RX 33

#define RFPIN 14
#define SIRENPIN 21
#define BTN 0
#endif

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