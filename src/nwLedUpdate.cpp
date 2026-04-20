#include <main.h>
#include <TinyGsm.h>

#define LED_TOGGLE_INTERVAL 500
#define CHECK_NW_SIGNAL_INTERVAL 10000

Ticker tGsmStatusLed;
extern TinyGsm modem;
int signalQuality = 0;

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
        signalQuality = modem.getSignalQuality();
        SerialMon.print("Signal quality: ");
        SerialMon.println(signalQuality);
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
    }
}