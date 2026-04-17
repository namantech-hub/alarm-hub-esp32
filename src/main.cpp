#include <main.h>
#include <ArduinoOTA.h>

Preferences pref;

RCSwitch rfin = RCSwitch();

Ticker tRfLed;

void turnOnRfLed()
{
  digitalWrite(SIGNAL_LED, LOW);
}

const char *ssid = "namantech";
const char *password = "NamanTech@123";
uint32_t last_ota_time = 0;

void setup()
{
  // initialize UARTs.
  SerialMon.begin(115200);
  pref.begin("alarmsys");
  if (!pref.getBool("init"))
  {
    pref.putBool("init", true);
    pref.putBool("armed", false);
    // numbers
    pref.putString("admin", "9403356476");
  }
  bArmed = pref.getBool("armed");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  SerialMon.println("Connecting to WiFi");

  // Init IOs
  pinMode(RFPIN, INPUT);
  pinMode(SIRENPIN, OUTPUT);
  digitalWrite(SIRENPIN, LOW);
  pinMode(NW_STATUS_LED, OUTPUT);
  digitalWrite(NW_STATUS_LED, HIGH);
  pinMode(SIGNAL_LED, OUTPUT);
  digitalWrite(SIGNAL_LED, HIGH);

  pinMode(ARMED_LED, OUTPUT);
  digitalWrite(ARMED_LED, HIGH);

  rfin.enableReceive(digitalPinToInterrupt(RFPIN));

  initNetworkLed();

  delay(10);

  // Init RF Receiver
  for (int i = 0; i < 5; i++)
  {
    digitalWrite(SIGNAL_LED, HIGH);
    digitalWrite(ARMED_LED, HIGH);
    delay(500);
    digitalWrite(SIGNAL_LED, LOW);
    digitalWrite(ARMED_LED, LOW);
    delay(500);
  }

  initGsm();
  setupOTA();
  SerialMon.println("Setup complete.");
}

void loop()
{
  updateNetworkLed();
  checkGsmSerial();

  // try to parse RF data
  unsigned long uCode = 0;
  if (rfin.available())
  {
    digitalWrite(SIGNAL_LED, HIGH);
    tRfLed.attach(1, turnOnRfLed);
    SerialMon.print("Received ");
    uCode = rfin.getReceivedValue();
    SerialMon.print(uCode);
    SerialMon.print(" / ");
    SerialMon.print(rfin.getReceivedBitlength());
    SerialMon.print("bit ");
    SerialMon.print("Protocol: ");
    SerialMon.println(rfin.getReceivedProtocol());
    rfin.resetAvailable();
  }
  updateState(uCode);

  // Update hardware
  digitalWrite(ARMED_LED, !bArmed);
}