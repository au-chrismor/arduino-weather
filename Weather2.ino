/*
 *  Weather Station V2.  The complete re-imagining...
 * 
 * The original Weather Station was one of my first Arduino projects, and I learned
 * a LOT since then.  Rather than just try and retrofit everything, sometimes it's
 * better to just make a clean start.
 * 
 *  Additionally, I have moved off Splunk and onto ThingSpeak because it's a cheaper
 * solution.  So all the old code around Splunk and Elastic has been removed rather than
 * using even more #ifdef rules.
 * 
 *  Note even more:
 * If you use the "Channel2 options to send more data to ThingSpeak, you can't compile this
 * on an Arduino Uno, because it exceeds the memory size if _DEBUG is declared.
 * This means you need a new ethernet setup as well, but if what you use is compatible with 
 * the Arduino Ethernet module, there _should_ be no changes needed in this code (YMMV, AMO18)
 * 
 */
#include "Weather2.h"

int readSolar(void) {
  int val = 0;
  Wire.begin();
  lux.begin();
  val = lux.readLightLevel();
  Wire.end();
  return val;
}

float readTemperature(void) {
  float val = 0.0;
  Wire.begin();
  while(!bme.begin()) {
    ;
  }
  val = bme.temp();
  Wire.end();
  return val;
}

float readHumidity(void) {
  float val = 0.0;
  Wire.begin();
  while(!bme.begin()) {
    ;
  }
  val = bme.hum();
  Wire.end();
  return val;
}

float readPressure(void) {
  float val = 0.0;
  Wire.begin();
  while(!bme.begin()) {
    ;
  }
  val = bme.pres(1);
  Wire.end();
  return val;

}

float readDewPoint(float temperature, float humidity) {
    return EnvironmentCalculations::DewPoint(temperature, humidity, true);
}

float readFrostPoint(float temperature, float humidity) {
    float dp = readDewPoint(temperature, humidity);
    return dp - temperature + 2671.02 / ((2954.61 / temperature) + 2.193665 * log(temperature) - 13.3448);
}

int readUV(void) {
    return map(analogRead(UV_PIN), 0, 1023, 0, (VREF * 1000));
}

int readUVIndex(int uv) {
    if(uv < 50)
        return 0;
    if(uv > 49 && uv < 227)
        return 1;
    if(uv > 226 && uv < 318)
        return 2;
    if(uv > 317 && uv < 408)
        return 3;
    if(uv > 407 && uv < 503)
        return 4;
    if(uv > 502 && uv < 606)
        return 5;
    if(uv > 605 && uv < 696)
        return 6;
    if(uv > 695 && uv < 795)
        return 7;
    if(uv > 794 && uv < 881)
        return 8;
    if(uv > 880 && uv < 976)
        return 9;
    if(uv > 975 && uv < 1079)
        return 10;
    else
        return 11;
}

int readWindDirection(void) {
    return map(analogRead(VANE_PIN), 0, 1023, 0, 359);
}

int readDust(void) {
    int value = 0;
    digitalWrite(DUST_LED, HIGH);
    delay(100);                     // Wake up
    value = analogRead(DUST_PIN);
    digitalWrite(DUST_LED, LOW);
    return value;
}

void isr_rain(void) {
  tipCount++;
  totalRainfall = tipCount * BUCKET_SIZE;
}

void isr_wind(void) {
  rotationCount++;
}

void isr_timer(void) {
    digitalWrite(LED_PIN, HIGH);
    timerCount++;
    readingTime++;
    if(timerCount == 5) {
        // We need to divide the count by 60 to get m/second
        // then multiply by 3.6 for Km/h
        windSpeed = rotationCount / 60 * ROT_VALUE * 3.6;
        rotationCount = 0;
        timerCount = 0;
    }
    digitalWrite(LED_PIN, LOW);
}

void blinkLED(unsigned int times) {
    for(int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(75);
        digitalWrite(LED_PIN, LOW);
        delay(75);
    }
}

void setup() {
    // If you use the Arduino Ethernet Shield (or a clone) on a Mega 2560, you
    // need to disable the SD card select.  If you don't all kinds of wierd network
    // things happen.  The next two lines fix this:
    pinMode(4,OUTPUT);
    digitalWrite(4,HIGH);
    
    pinMode(LED_PIN, OUTPUT);
    pinMode(DATA_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    analogReference(DEFAULT);   // Make sure we know our vRef
    
    // If using the Freetronics EtherTen board,
    // the W5100 takes a bit longer to come out of reset
    delay(5000);   // This should be plenty

#ifdef _DEBUG
    Serial.begin(19200);
    while(!Serial) {
        blinkLED(1);
        delay(10);    // Wait for serial port
    }
    Serial.println("\n\nCold Start");
#endif

    blinkLED(2);

#ifdef _DEBUG
    Serial.println("\nBME280 is initialised!");
#endif

    byte mac[] = {
        0x02, 0x60, 0x8C, 0x01, 0x02, 0x03
    };
#ifdef _DEBUG    
        Serial.println("Waiting for DHCP");
#endif
    while(Ethernet.begin(mac) != 1) {
#ifdef _DEBUG
      Serial.print(".");
#endif
        blinkLED(3);
        delay(1000);
    }
#ifdef _DEBUG
    Serial.print("\nNetwork is active.  Address: ");
    Serial.println(Ethernet.localIP());
    Serial.print("DNS: ");
    Serial.println(Ethernet.dnsServerIP());
#endif

    blinkLED(4);
    pinMode(DUST_LED, OUTPUT);
    digitalWrite(DUST_LED, LOW);    // Turn off dust sensor
    digitalWrite(DATA_PIN, LOW);
    
    // Set all the globals to a valid starting point.
    totalRainfall = 0;
    tipCount = 0;
    contactTime = 0;
    contactBounceTime = 0;
    rotationCount = 0;
    timerCount = 0;
    readingTime = 0;
    totalMinutes = 0;   // Use this to determine 24 hours
    
    // Initialise the interrupt for the rain gauge
    blinkLED(5);
    pinMode(RAIN_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(RAIN_PIN), isr_rain, FALLING);
#ifdef _DEBUG
    Serial.println("Attached Rain Sensor");
#endif

    // Initialise the interrupt for wind speed
    blinkLED(6);
    pinMode(SPEED_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(SPEED_PIN), isr_wind, FALLING);
#ifdef _DEBUG
    Serial.println("Attached Speed Sensor");
#endif
    
    // Set up a timer for 1/2 second intervals
    blinkLED(7);
    Timer1.initialize(500000);
    Timer1.attachInterrupt(isr_timer); 
#ifdef _DEBUG
    Serial.println("Attached Timer");
#endif

    blinkLED(8);
    ThingSpeak.begin(client);
#ifdef _DEBUG
    Serial.println("Started ThingSpeak");
#endif
    
    sei();    // Enable interrupts
}

void loop() {
    float temperature = 0.0;
    float humidity = 0.0;
    float uv = 0.0;
    float windDir = 0.0;
    float pressure = 0.0;
    float solar = 0.0;
    float dewPoint = 0.0;
    float frostPoint = 0.0;
    int uVIndex = 0;
    float dust = 0.0;

    Ethernet.maintain();
    
    // Have our 60 seconds expired yet?
    if(readingTime > 119) {
      digitalWrite(LED_PIN, HIGH);
      temperature = readTemperature();
      humidity = readHumidity();
      uv = readUV();
      windDir = readWindDirection();
      pressure = readPressure();
      solar = readSolar();
      dewPoint = readDewPoint(temperature, humidity);
      frostPoint = readFrostPoint(temperature, humidity);
      uVIndex = readUVIndex(uv);
      dust = readDust();
        
#ifdef _DEBUG
        Serial.print("Temperature: ");
        Serial.println(temperature);
        Serial.print("Humidity: ");
        Serial.println(humidity);
        Serial.print("UV: ");
        Serial.println(uv);
        Serial.print("Wind Speed: ");
        Serial.println(windSpeed);
        Serial.print("Wind Direction: ");
        Serial.println(windDir);
        Serial.print("Rainfall: ");
        Serial.println(totalRainfall);
        Serial.print("Barometer: ");
        Serial.println(pressure);
        Serial.print("Solar: ");
        Serial.println(solar);
        Serial.print("Dewpoint: ");
        Serial.println(dewPoint);
        Serial.print("Frostpoint: ");
        Serial.println(frostPoint);
        Serial.print("UV Index: ");
        Serial.println(uVIndex);
        Serial.print("Dust Sensor: ");
        Serial.println(dust);
#endif   
      digitalWrite(LED_PIN, LOW);
      digitalWrite(DATA_PIN, HIGH);
#ifdef _DEBUG
      Serial.println("Starting send");
#endif
      ThingSpeak.setField(1, temperature);
      ThingSpeak.setField(2, humidity);
      ThingSpeak.setField(3, uv);
      ThingSpeak.setField(4, windSpeed);
      ThingSpeak.setField(5, windDir);
      ThingSpeak.setField(6, (float)totalRainfall);
      ThingSpeak.setField(7, pressure);
      ThingSpeak.setField(8, solar);
      int ret = ThingSpeak.writeFields(channel1Id, api1Key);
#ifdef _DEBUG      
      Serial.print("Return from writeFields: ");
      Serial.println(ret);
#endif
#ifdef _CHANNEL2
        ThingSpeak.setField(FIELD_DP, dewPoint);
        ThingSpeak.setField(FIELD_FP, frostPoint);
        ThingSpeak.setField(FIELD_UVI, uVIndex);
        ThingSpeak.setField(FIELD_DUST, dust);
        ThingSpeak.writeFields(channel2Id, api2Key);
#endif        
#ifdef _DEBUG
        Serial.println("Sending done");
#endif
        digitalWrite(DATA_PIN, LOW);
        readingTime = 0;
        // General housekeeping stuff
        totalMinutes++;
        if(totalMinutes > 1439) { // 24 hours
            totalRainfall = 0;
            totalMinutes = 0;
        }
    }
}
