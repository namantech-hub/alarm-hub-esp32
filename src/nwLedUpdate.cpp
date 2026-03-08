#include <main.h>
#include <TinyGsm.h>

#define LED_TOGGLE_INTERVAL 500
#define CHECK_NW_SIGNAL_INTERVAL 5000

Ticker tGsmStatusLed;
extern TinyGsm modem;

void toggleNwLed()
{
    digitalWrite(NW_STATUS_LED, !digitalRead(NW_STATUS_LED));
}

void initNetworkLed()
{
    pinMode(NW_STATUS_LED, OUTPUT);
    tGsmStatusLed.attach_ms(500, toggleNwLed);
}

void updateNetworkLed()
{
    static uint32_t prevMilis;
    uint32_t milis = millis();

    if ((milis - prevMilis) > CHECK_NW_SIGNAL_INTERVAL)
    {
        int signalQuality = modem.getSignalQuality();
        if (signalQuality >= 99 || signalQuality < 10)
        {
            tGsmStatusLed.attach_ms(LED_TOGGLE_INTERVAL, toggleNwLed);
        }
        else
        {
            tGsmStatusLed.detach();
            digitalWrite(NW_STATUS_LED, LOW);
        }
        prevMilis = milis;
        // Serial.println("Signal quality: " + String(modem.getSignalQuality()));
    }
}