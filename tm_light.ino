#include "config.h"
#include <Arduino.h>
#include <MFRC522.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <TinyGsmClient.h>

MFRC522 mfrc522;

#ifdef DEBUG_DUMP_AT_COMMAND
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger, AM7020_RESET);
#else
// 建立 AM7020 modem（設定 Serial 及 EN Pin）
TinyGsm modem(SerialAT, AM7020_RESET);
#endif

// 在 modem 架構上建立 Tcp Client
TinyGsmClient tcpClient(modem);
// 在 Tcp Client 架構上建立 MQTT Client
PubSubClient mqttClient(MQTT_BROKER, MQTT_PORT, tcpClient);

void mqttCallback(char *topic, byte *payload, unsigned int len);
void mqttConnect(void);
void nbConnect(void);

void setup()
{
    SerialMon.begin(MONITOR_BAUDRATE);
    SerialAT.begin(AM7020_BAUDRATE);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    // AM7020 NBIOT 連線基地台
    nbConnect();
    // 設定 MQTT KeepAlive 為270秒
    mqttClient.setKeepAlive(270);
    mqttClient.setCallback(mqttCallback);
}

void loop()
{
    // 檢查 MQTT Client 連線狀態
    if (!mqttClient.connected()) {
        // 檢查 NBIOT 連線狀態
        if (!modem.isNetworkConnected()) {
            nbConnect();
        }
        SerialMon.println(F("=== MQTT NOT CONNECTED ==="));
        mqttConnect();
        mqttClient.subscribe(MQTT_TOPIC_CLASS_NAME);
    }

    // mqtt handle
    mqttClient.loop();
}

/**
 * MQTT Callback
 */
void mqttCallback(char *topic, byte *payload, unsigned int len)
{
    SerialMon.println(topic);
    SerialMon.write(payload, len);
    SerialMon.println();
    if (strncmp("on", (char *)payload, (size_t)len) == 0) {
        digitalWrite(LED_PIN, HIGH);
    } else if (strncmp("off", (char *)payload, (size_t)len) == 0) {
        digitalWrite(LED_PIN, LOW);
    } else {
    }
}

/**
 * MQTT Client 連線
 */
void mqttConnect(void)
{
    SerialMon.print(F("Connecting to "));
    SerialMon.print(MQTT_BROKER);
    SerialMon.print(F("..."));

    // Connect to MQTT Broker
    while (!mqttClient.connect("AM7020_MQTTID_TM_LIGHT_20210303", MQTT_USERNAME, MQTT_PASSWORD)) {
        SerialMon.println(F(" fail"));
    }
    SerialMon.println(F(" success"));
}

/**
 * AM7020 NBIOT 連線基地台
 */
void nbConnect(void)
{
    SerialMon.println(F("Initializing modem..."));
    while (!modem.init() || !modem.nbiotConnect(APN, BAND)) {
        SerialMon.print(F("."));
    };

    SerialMon.print(F("Waiting for network..."));
    while (!modem.waitForNetwork()) {
        SerialMon.print(F("."));
    }
    SerialMon.println(F(" success"));
}
