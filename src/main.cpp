#include <main.h>
#include <TinyGsmClient.h>
#include <StreamDebugger.h>

#define SerialMon Serial
#define SerialAT Serial1

Preferences pref;

StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);

RCSwitch rfin = RCSwitch();

void setup()
{
  // initialize UARTs.
  Serial.begin(115200);
  pref.begin("alarmsys");
  if (!pref.getBool("init"))
  {
    pref.putBool("init", true);
    pref.putBool("armed", false);
    // numbers
    pref.putString("admin", "9403356476");
  }
  bArmed = pref.getBool("armed");
  // gsm.begin(PIN_TX, PIN_RX);

  // Init IOs
  pinMode(SIRENPIN, OUTPUT);
  digitalWrite(SIRENPIN, LOW);

  pinMode(ARMED_LED, OUTPUT);
  digitalWrite(ARMED_LED, HIGH);

  rfin.enableReceive(digitalPinToInterrupt(RFPIN));

  initNetworkLed();

  delay(10);

  // Init RF Receiver
  Serial.println("Initializing LoRa Receiver");
  // override the default CS, reset, and IRQ pins (optional)

  SerialMon.println("Initializing modem...");
  pinMode(RST_PIN, OUTPUT);
  delay(1000);
  digitalWrite(RST_PIN, LOW);
  delay(1000);
  digitalWrite(RST_PIN, HIGH);
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
  delay(10000); // Wait for modem to stabilize
  bool bModemInit = modem.init();
  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);
  modem.sendAT("+CMGF=1"); // Set SMS to text mode
  modem.waitResponse();
  //     Serial2.printf("AT+CNMI=1,2,0,1,0\r\n"); // SMS details
  modem.sendAT("+CNMI=1,2,0,1,0"); // SMS details
  modem.waitResponse();
  modem.sendAT("E1");
  modem.waitResponse();

  Serial.println("Setup complete.");
}

void loop()
{
  updateNetworkLed();
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

  // try to parse RF data
  unsigned long uCode = 0;
  if (rfin.available())
  {
    Serial.print("Received ");
    uCode = rfin.getReceivedValue();
    Serial.print(uCode);
    Serial.print(" / ");
    Serial.print(rfin.getReceivedBitlength());
    Serial.print("bit ");
    Serial.print("Protocol: ");
    Serial.println(rfin.getReceivedProtocol());
    rfin.resetAvailable();
  }
  updateState(uCode);

  // Send boot message if not sent already and 30 seconds have passed since boot
  static bool bootMsgSent = 0;
  if (!bootMsgSent && millis() > 30000)
  {
    // Send boot message
    int bootCnt = pref.getInt("bootCnt", 0);
    bootCnt++;
    pref.putInt("bootCnt", bootCnt);
    String number = pref.getString("admin");
    String message = "Alarm System ESP32S3 Booted " + String(bootCnt) + " times.";
    modem.sendSMS(number, message);
    bootMsgSent = true;
  }

  // Update hardware
  digitalWrite(ARMED_LED, !bArmed);
}