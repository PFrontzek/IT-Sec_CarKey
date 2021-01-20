#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "mbedtls/md.h"
#include <EEPROM.h>
#define LED_BUILTIN 2
#define PIN_DOOR_OPEN 27
#define PIN_DOOR_CLOSE 26
#define ACTION_DOOR_OPEN 1
#define ACTION_DOOR_CLOSE -1
#define ACTION_KOFFERRAUM 3

#define MAX_TIME_DIFF_S 10
#define KEY_SIZE 32

typedef struct MyPacket
{
  uint16_t length = 3 * sizeof(uint16_t) + 32 * sizeof(uint8_t) + 1 * sizeof(int8_t);
  uint16_t time;
  uint16_t seq;
  int8_t action;
  uint8_t signature[32];
} MyPacket;

uint8_t carkey_key[KEY_SIZE];
volatile bool change = false;
const char *ssid = "ssid";
const char *password = "password";
const char *ntpServer = "pool.ntp.org";

const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;
struct tm timeinfo;

WiFiClient client;
mbedtls_md_context_t ctx;
mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
MyPacket packet = {};

void IRAM_ATTR openDoor()
{
  if (!change)
  {
    packet.action = ACTION_DOOR_OPEN;
    change = true;
  }
}

void IRAM_ATTR closeDoor()
{
  if (!change)
  {
    packet.action = ACTION_DOOR_CLOSE;
    change = true;
  }
}

void transmit(void *p, size_t length)
{
  client.connect("10.1.0.29", 10001);
  client.write((byte *)&packet.length, packet.length);
  client.flush();
  client.stop();
}

void populate_key()
{
  // srand(time(NULL));
  for (int i = 0; i < KEY_SIZE; i++)
  {
    // EEPROM.write(i, rand());
    carkey_key[i] = EEPROM.read(i);
  }
  EEPROM.commit();
}

void populate_signature()
{
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, carkey_key, sizeof(carkey_key));
  mbedtls_md_update(&ctx, (byte *)&packet, packet.length - sizeof(packet.signature));
  mbedtls_md_finish(&ctx, packet.signature);
  mbedtls_md_free(&ctx);
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_DOOR_OPEN, INPUT_PULLUP);
  pinMode(PIN_DOOR_CLOSE, INPUT_PULLUP);
  EEPROM.begin(32);
  delay(100);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  populate_key();
  attachInterrupt(PIN_DOOR_OPEN, openDoor, FALLING);
  attachInterrupt(PIN_DOOR_CLOSE, closeDoor, FALLING);
}

void loop()
{
  if (change)
  {
    packet.seq++;
    getLocalTime(&timeinfo);
    packet.time = timeinfo.tm_sec + 60 * timeinfo.tm_min + 60 * 60 * (timeinfo.tm_hour / 4);
    populate_signature();
    transmit(&packet, packet.length);
    delay(500);
    change = false;
  }
}