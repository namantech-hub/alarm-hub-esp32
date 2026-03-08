#include <main.h>

extern TinyGsm modem;

bool bArmed = false;
// Send unknown RF code to admin
bool bSendUnknownCode = false;

String szLastMsg;
Ticker tSirenTimout;
Ticker tSirenSeq1, tSirenSeq2, tSirenSeq3;

enum state_types_t
{
    ARM = 1,
    DISARM,
    INTRUSION,
    PANIC,
};

char iSeq = '6';
uint8_t iType = 0;

void sirenOff()
{
    digitalWrite(SIRENPIN, LOW);
}

void sirenOn()
{
    digitalWrite(SIRENPIN, HIGH);
}

void sendSms(char iNo, String message)
{
    String index = "n" + String(iNo);
    String number = pref.getString(index.c_str());
    if (number.length() == 10)
    {
        modem.sendSMS(number, message);
    }
}

void arm()
{
    sirenOff();
    if (bArmed)
    {
        Serial.println("Already armed");
        return;
    }
    Serial.println("Armed");
    bArmed = true;
    iType = state_types_t::ARM;
    iSeq = '1';
    sirenOn();
    tSirenSeq1.once_ms(100, sirenOff);
    pref.putBool("armed", bArmed);
}

void disarm()
{
    sirenOff();
    if (!bArmed)
    {
        Serial.println("Already disarmed");
        return;
    }
    Serial.println("Disarmed");
    bArmed = false;
    iType = state_types_t::DISARM;
    iSeq = '1';
    sirenOn();
    tSirenSeq1.once_ms(100, sirenOff);
    tSirenSeq2.once_ms(400, sirenOn);
    tSirenSeq3.once_ms(500, sirenOff);
    pref.putBool("armed", bArmed);
}

void intrusion()
{
    if (digitalRead(SIRENPIN))
    {
        Serial.println("Already triggered");
        return;
    }
    Serial.println("intrusion detected!");
    iType = state_types_t::INTRUSION;
    iSeq = '1';
    sirenOn();
    tSirenTimout.once(120, sirenOff);
}

void panic()
{
    if (digitalRead(SIRENPIN))
    {
        Serial.println("Already triggered");
        return;
    }
    iType = state_types_t::PANIC;
    iSeq = '1';
    Serial.println("Panic!");
    sirenOn();
}

void updateState(uint32_t rfdata)
{
    static uint32_t nextMillis = 0;
    static uint32_t prevRfData = 0;
    if (rfdata /*&& prevRfData != rfdata*/)
    {
        Serial.printf("Rf Data:%X\n", rfdata);
        prevRfData = rfdata;
        bool bCodeFound = false;
        for (char i = '1'; i <= '9'; i++)
        {
            // Check for disarm code
            String s = "d" + String(i);
            if (pref.getUInt(s.c_str()) == rfdata)
            {

                szLastMsg = String(rfdata, HEX);
                bCodeFound = true;
                disarm();
                break;
            }

            // Check for arm code
            s = "a" + String(i);
            if (pref.getUInt(s.c_str()) == rfdata)
            {
                szLastMsg = String(rfdata, HEX);
                bCodeFound = true;
                arm();
                break;
            }

            // Check for sensor code
            s = "s" + String(i);
            if (pref.getUInt(s.c_str()) == rfdata && bArmed)
            {
                szLastMsg = String(rfdata, HEX);
                bCodeFound = true;
                intrusion();
                break;
            }

            // Check for panic code
            s = "p" + String(i);
            if (pref.getUInt(s.c_str()) == rfdata)
            {
                szLastMsg = String(rfdata, HEX);
                bCodeFound = true;
                panic();
                break;
            }
        }

        Serial.printf("Code found: %d, debug: %d, iSeq: %c\n", bCodeFound, bSendUnknownCode, iSeq);

        // If unknwon code & code is not found
        if (bSendUnknownCode && !bCodeFound && iSeq > '6')
        {
            String number = pref.getString("admin");
            String message = String("Unknown Code:") + String(rfdata, HEX);
            modem.sendSMS(number, message);
            nextMillis = millis() + 10000;
        }
    }

    if (iSeq < '7')
    {
        uint32_t ms = millis();
        if (ms > nextMillis)
        {
            String index = "n" + String(iSeq);
            String number = iSeq < '6' ? pref.getString(index.c_str()) : pref.getString("admin");
            iSeq++;

            if (number.length() == 10 && number != "0000000000")
            {
                switch (iType)
                {
                case state_types_t::ARM:
                    if (iSeq < '6')
                        modem.sendSMS(number, "System is armed");
                    else
                        modem.sendSMS(number, "System is armed " + szLastMsg);
                    nextMillis = millis() + 10000;
                    break;
                case state_types_t::DISARM:
                    if (iSeq < '6')
                        modem.sendSMS(number, "System is disarmed");
                    else
                        modem.sendSMS(number, "System is disarmed " + szLastMsg);
                    nextMillis = millis() + 10000;
                    break;
                case state_types_t::INTRUSION:
                    if (iSeq < '6')
                    {
                        modem.callHangup();
                        delay(500);
                        modem.callNumber(number);
                    }
                    else
                        modem.sendSMS(number, "Intrusion detected " + szLastMsg);
                    nextMillis = millis() + 30000;
                    break;
                case state_types_t::PANIC:
                    if (iSeq < '6')
                        modem.sendSMS(number, "Panic alarm activated");
                    else
                        modem.sendSMS(number, "Panic alarm activated " + szLastMsg);
                    nextMillis = millis() + 30000;
                    break;
                }
            }
        }
    }
}