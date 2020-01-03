#ifndef __WEATHER2_H_INCLUDED__
#include <SPI.h>
//#include <SD.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <BME280.h>
#include <BME280I2C.h>
#include <EnvironmentCalculations.h>
#include <Wire.h>
#include <BH1750.h>
#include <ThingSpeak.h>
#include <TimerOne.h>


#define _DEBUG
#define _CHANNEL2

#define BUCKET_SIZE 0.1     // Rain in mm per tipping event
#define ROT_VALUE   1.0     // Distance of one rotation
#define RAIN_PIN    18
#define SPEED_PIN   19
#define INTR4       20
#define INTR5       21
#define UV_PIN      (A0)
#define VANE_PIN    (A1)
#define DUST_PIN    (A2)
#define LED_PIN     35
#define DATA_PIN    33
#define DUST_LED    31
#define BH1750_ADDR 0x23    // BH1750 Solar Sensor I2C address
#define VREF        3.3

#define FIELD_TEMP   1
#define FIELD_HUM    2
#define FIELD_UV     3
#define FIELD_SPEED  4
#define FIELD_DIR    5
#define FIELD_RAIN   6
#define FIELD_PRES   7
#define FIELD_LIGHT  8
#ifdef _CHANNEL2
#define FIELD_DP     1
#define FIELD_FP     2
#define FIELD_UVI    3
#define FIELD_DUST   4
#define FIELD_GEIG   5
#endif

// ThingSpeak Settings
const char *server = "api.thingspeak.com";
const char *  api1Key ="XXXXXXXXXXXXXXXX";
unsigned long channel1Id = 000000;
#ifdef _CHANNEL2
const char *  api2Key ="XXXXXXXXXXXXXXXX";
unsigned long channel2Id = 000000;
#endif

BME280I2C bme;
EthernetClient client;
BH1750 lux;

float windSpeed;
volatile unsigned long tipCount;
unsigned long contactTime;
unsigned long contactBounceTime;
volatile unsigned long totalRainfall;
volatile unsigned long rotationCount;
unsigned int timerCount;
unsigned int readingTime;
unsigned int totalMinutes;

void initSolar(void);
int readSolar(void);
float readTemperature(void);
float readHumidity(void);
float readPressure(void);
float readDewPoint(float temperature, float humidity);
float readFrostPoint(float temperature, float humidity);
int readUV(void);
int readUVIndex(int UV);
int readWindDirection(void);
int readDust(void);
int readGeiger(void);

void blinkLED(unsigned int times);

void isr_rain(void);
void isr_wind(void);
void isr_timer(void);

#define __WEATHER2_H_INCLUDED__
#endif
