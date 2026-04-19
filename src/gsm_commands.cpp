#include <main.h>
#include <TinyGsmClient.h>
#include <StreamDebugger.h>

StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);

extern int signalQuality;

void initGsm()
{
    SerialMon.println("Initializing modem...");
    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
    modem.init();
    String modemInfo = modem.getModemInfo();
    SerialMon.print("Modem Info: ");
    SerialMon.println(modemInfo);
    modem.sendAT("+CMGF=1"); // Set SMS to text mode
    modem.waitResponse();
    modem.sendAT("+CNMI=1,2,0,1,0"); // SMS details
    modem.waitResponse();
    modem.sendAT("E1");
    modem.waitResponse();
    modem.sendAT("+CLIP=1"); // Enable caller ID notification
    modem.waitResponse();
}

void phoneCmd(String szFrom, String szCmd)
{
    szCmd.replace("phone", "");
    szCmd.trim();
    char id = szCmd[0];
    String num = szCmd.substring(2);
    num.trim();

    if (id > '0' && id < '6')
    {
        String memId = "n" + String(id);
        if (num.length() == 10)
            pref.putString(memId.c_str(), num);
        String phone = pref.getString(memId.c_str());
        Serial.printf("number %s = %s\n", memId.c_str(), phone.c_str());
        modem.sendSMS(szFrom, "phone " + String(id) + " " + phone);
    }
    else if (id == '6')
    {
        String memId = "admin";
        String curAdminNo = pref.getString(memId.c_str());
        if (szFrom != curAdminNo)
        {
            modem.sendSMS(curAdminNo, szFrom + " tried changing admin to" + num);
        }
        else
        {
            if (num.length() == 10)
                pref.putString(memId.c_str(), num);
            num = pref.getString(memId.c_str());
            Serial.printf("number %s = %s\n", memId.c_str(), num.c_str());
            modem.sendSMS(curAdminNo, "phone " + String(id) + " " + num);
        }
    }
    else
    {
        Serial.printf("invalid phone command");
        modem.sendSMS(szFrom, "invalid phone id");
        return;
    }
}

void sensorCmd(String szFrom, String szCmd)
{
    szCmd.replace("sensor", "");
    szCmd.trim();

    // sensor a1 5ba9a1
    char id, type;
    String code;

    if (szCmd.length() < 2)
    {
        Serial.printf("Invalid command %d\n", szCmd.length());
        modem.sendSMS(szFrom, "Invalid command");
        return;
    }
    type = szCmd[0];
    id = szCmd[1];

    if (szCmd.length() > 3 && szCmd.length() < 10)
    {
        code = szCmd.substring(3);
    }
    // Eg:
    // sensor a arm abcd
    // sensor b disarm abcd
    if (type != 'a' && type != 'd' && type != 's' && type != 'p')
    {
        Serial.println("Invalid sensor type");
        modem.sendSMS(szFrom, "Invalid sensor type");
        return;
    }

    if (id < '1' || id > '9')
    {
        Serial.println("Sensor id out of index");
        modem.sendSMS(szFrom, "Sensor id out of index");
        return;
    }

    String memId = String(type) + String(id);
    uint32_t iCode = pref.getUInt(memId.c_str());
    if (code.length() < 1 || code.length() > 6)
    {
        Serial.println("Invalid sensor code");
    }
    else
    {
        iCode = strtoul(code.c_str(), NULL, 16);
        pref.putUInt(memId.c_str(), iCode);
        Serial.printf("Ok. id=%s szCode=%s, code=%X\n", memId.c_str(), code.c_str(), iCode);
    }
    // send SMS
    String payload = "sensor " + memId + " " + String(iCode, HEX);
    Serial.println(payload);
    modem.sendSMS(szFrom, payload);
}

void getState(String szFrom)
{
    String payload = bArmed ? "Armed" : "Disarmed";
    payload += "\nSignal Quality: " + String(signalQuality);

    modem.sendSMS(szFrom, payload);
}

void processPhoneCmd(String szFrom, String szCmd)
{
    // Serial.printf("num=%s, cmd=%s\n", gsm.getNumber(), gsm.getCommand());
    // Check if szFrom is from known numbers
    szCmd.toLowerCase();
    bool bKnownNumber = false;
    if (szFrom == pref.getString("admin"))
        bKnownNumber = true;
    else
        for (char i = '1'; i < '6'; i++)
        {
            String index = "n" + String(i);
            if (pref.getString(index.c_str()) == szFrom)
            {
                bKnownNumber = true;
                break;
            }
        }

    if (!bKnownNumber)
    {
        // forward msg reveived from unknown number
        String payload = "From: " + szFrom + "\n Cmd: " + szCmd;
        Serial.println(payload);
        modem.sendSMS(pref.getString("admin"), payload);
        return;
    }

    Serial.printf("szCmd=%s\n", szCmd.c_str());
    szLastMsg = "sms " + szFrom;
    if (szCmd == "call")
    {
        modem.callHangup();
        szLastMsg = "call " + szFrom;
        arm();
    }
    else if (szCmd.startsWith("phone"))
        phoneCmd(szFrom, szCmd);
    else if (szCmd.startsWith("sensor"))
        sensorCmd(szFrom, szCmd);
    else if (szCmd.startsWith("arm"))
        arm();
    else if (szCmd.startsWith("disarm"))
        disarm();
    else if (szCmd.startsWith("panic"))
        panic();
    else if (szCmd.startsWith("state") || szCmd.startsWith("status"))
        getState(szFrom);
    else if (szCmd.startsWith("debug"))
    {
        bSendUnknownCode = true;
        modem.sendSMS(szFrom, "Debug enabled\n Signal Quality: " + String(signalQuality));
    }
    else if (szCmd.startsWith("nodebug"))
    {
        bSendUnknownCode = false;
        modem.sendSMS(szFrom, "Debug disabled\n Signal Quality: " + String(signalQuality));
    }
}

void checkGsmSerial()
{
    // Check if SMS is available
    if (SerialAT.available())
    {
        String smsData = SerialAT.readStringUntil('\n');
        SerialMon.print("Received data: ");
        SerialMon.println(smsData);
        if (smsData.indexOf("+CMT:") != -1) // New SMS received
        {
            String senderNumber = smsData.substring(smsData.indexOf("\"") + 1, smsData.indexOf("\",", smsData.indexOf("\"") + 1));
            // Remove country code if present
            if (senderNumber.startsWith("+91"))
            {
                senderNumber = senderNumber.substring(3);
            }
            String message = SerialAT.readStringUntil('\n'); // Read the actual message
            SerialMon.print("SMS from ");
            SerialMon.print(senderNumber);
            SerialMon.print(": ");
            SerialMon.println(message);
            processPhoneCmd(senderNumber, message);
        }
        // Also check if call & get caller's number
        else if (smsData.indexOf("RING") != -1)
        {
            // Incoming call detected, get caller number
            modem.sendAT("+CLCC"); // List current calls
            String response;
            modem.waitResponse(1000, response);
            if (response.indexOf("+CLCC:") != -1)
            {
                int numStart = response.indexOf("\"") + 1;
                int numEnd = response.indexOf("\"", numStart);
                String callerNumber = response.substring(numStart, numEnd);
                // Remove country code if present
                if (callerNumber.startsWith("+91"))
                {
                    callerNumber = callerNumber.substring(3);
                }
                SerialMon.print("Incoming call from: ");
                SerialMon.println(callerNumber);
                processPhoneCmd(callerNumber, "call");
            }
        }
    }

    // Send boot message if not sent already and 30 seconds have passed since boot
    static bool bootMsgSent = 0;
    if (!bootMsgSent && millis() > 30000)
    {
        // Send boot message
        int bootCnt = pref.getInt("bootCnt", 0);
        bootCnt++;
        pref.putInt("bootCnt", bootCnt);
        String number = pref.getString("admin");
        String message = "alarm-hub-esp32 v" + String(FW_VERSION) + " booted " + String(bootCnt) + " times.";
        message += "\nSignal Quality: " + String(signalQuality);
        modem.sendSMS(number, message);
        SerialMon.println("Sent boot message to " + number + " : " + message);
        bootMsgSent = true;
    }
}