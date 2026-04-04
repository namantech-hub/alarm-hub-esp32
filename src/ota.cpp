#include <main.h>
#include <ArduinoOTA.h>

void otaTask(void *pvParameters)
{
  while (1)
  {
    ArduinoOTA.handle();
    delay(10);
  }
}

void setupOTA()
{
  ArduinoOTA.setHostname("alarm-hub-esp32");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    SerialMon.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    SerialMon.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    SerialMon.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    SerialMon.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      SerialMon.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      SerialMon.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      SerialMon.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      SerialMon.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      SerialMon.println("End Failed");
    }
   });
  ArduinoOTA.begin();

  xTaskCreate(otaTask, "OTA Task", 8192, NULL, 1, NULL);
}