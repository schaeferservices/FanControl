/*
    Name:       FanControl
    Version:    2.1
    Created:	08/2020
    Author:     Daniel Schäfer
*/

/**************************************************/
/**                 Project settings             **/
/**************************************************/
/**/     #define DEBUG 1                        /**/
/**/     #define TEST_SPEAKER 0                 /**/
/**/     #define SERIAL_BAUD_RATE 115200        /**/
/**/     #define MILLIS_SLEEP_LOOP 1000         /**/
/**/     #define OTA_ENABLE 1                   /**/
/**************************************************/

#include "PinSetup.h"
#include "NetworkSettings.h"
#include "AdditionalDefinitions.h"

#include "libraries/DHT_sensor_library/DHT.h"

// include section only ESP8266
#ifdef ESP8266

#include <ESP8266WiFi.h>
#include <Hash.h>
#include "libraries/ESPAsyncTCP/ESPAsyncTCP.h"
#include "libraries/ESPAsyncWebServer/ESPAsyncWebServer.h"
#include <ArduinoOTA.h>

AsyncWebServer server(WEB_API_PORT);

#endif

DHT dht(PIN_DHT11_DATA, DHT11);

int counter_seconds = 0;
uint16_t counter_rpm[2] = { 0 }, rpm[2] = { 0 }, rpm_set = 0;
float avgTemp = 0;
SystemState state = STATUS_OK;
uint64_t previousMillis = 0;
float temp = 0, hum = 0;

void setup()
{
#ifdef _DEBUG
    Serial.begin(SERIAL_BAUD_RATE);
#endif

    dht.begin();

    pinMode(PIN_PWM_1, OUTPUT);
    pinMode(PIN_PWM_2, OUTPUT);
    pinMode(PIN_SPEAKER, OUTPUT);
    pinMode(PIN_DHT11_DATA, INPUT);
    pinMode(PIN_RPM_1, INPUT);
    pinMode(PIN_RPM_2, INPUT);

    //digitalWrite(PIN_RPM_1, HIGH);
    //digitalWrite(PIN_RPM_2, HIGH);
    rpm_interrupts_enable();

#ifdef ESP8266

    initWebApi();

#ifdef OTA_ENABLE

    // setup OTA
    ArduinoOTA.setHostname(OTA_HOSTNAME);  
    ArduinoOTA.begin(); 
    ArduinoOTA.onStart([]() {
        server.end();
    });

#endif

#endif
}

void _main()
{
    temp = dht.readTemperature();
    hum = dht.readHumidity();

    log_print("Temp: ");
    log_print(temp);
    log_print(" Hum: ");
    log_print(hum);
    log_print("%");

    // calc rpm
    calc_rpm(RPM_1);
    calc_rpm(RPM_2);
    log_println();

    if (counter_seconds == 0)
    {
        avgTemp = temp;
    }

    counter_seconds++;
    avgTemp = avgTemp + temp;

    if (counter_seconds >= 10)
    {
        avgTemp = avgTemp / 11;

        log_print("avgTemp: ");
        log_println(avgTemp);

        uint16_t rpm_set = 0;
        if (avgTemp >= 20)
        {
            rpm_set = avgTemp * 8.5 - 170;

            if (rpm_set < 28)
            {
                rpm_set = 28;
            }

            analogWrite(PIN_PWM_1, rpm_set);
            analogWrite(PIN_PWM_2, rpm_set);
        }
        else if (isnan(avgTemp))
        {
            rpm_set = 255;
            analogWrite(PIN_PWM_1, rpm_set);
            analogWrite(PIN_PWM_2, rpm_set);
            state = ERROR_TEMP;
        }
        else
        {
            rpm_set = 0;
            analogWrite(PIN_PWM_1, rpm_set);
            analogWrite(PIN_PWM_2, rpm_set);
        }

        log_print("RPM_set: ");
        log_print(rpm_set / 2.55);
        log_println("%");

        counter_seconds = 0;
    }
    else if (isnan(temp))
    {
        rpm_set = 255;
        analogWrite(PIN_PWM_1, rpm_set);
        analogWrite(PIN_PWM_2, rpm_set);
        state = ERROR_TEMP;

        log_println();
        log_print("RPM_set: ");
        log_print(rpm_set / 2.55);
        log_println("%");
    }

    // set alarm state
    if (state != STATUS_OK || TEST_SPEAKER)
    {
        digitalWrite(PIN_SPEAKER, HIGH);
    }
    else
    {
        digitalWrite(PIN_SPEAKER, LOW);
    }
}

void loop()
{
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= MILLIS_SLEEP_LOOP)
    {
        previousMillis = currentMillis;
        _main();
    }

#ifdef ESP8266
#ifdef OTA_ENABLE
    ArduinoOTA.handle();
#endif
#endif
}

void calc_rpm(uint8_t num)
{
    rpm_interrupts_disable();

    rpm[num] = counter_rpm[num] * (60 / 2);

    log_print(" RPM_FAN_");
    log_print(num);
    log_print(": ");
    log_print(rpm[num] / 18);
    log_print("% ");

    if (abs((rpm_set / 2.55) - (rpm[num] / 18)) > 10 || abs((rpm_set / 2.55) - (rpm[num] / 18)) < 10)
    {
        state = ERROR_RPM;
    }

    counter_rpm[num] = 0;

    rpm_interrupts_enable();
}

void rpm_interrupts_enable()
{
    attachInterrupt(PIN_RPM_1, isr_rpm_1, FALLING);
    attachInterrupt(PIN_RPM_2, isr_rpm_2, FALLING);
}

void rpm_interrupts_disable()
{
    detachInterrupt(PIN_RPM_1);
    detachInterrupt(PIN_RPM_2);
}

#ifdef ESP8266
void initWebApi()
{
    // Connect to Wi-Fi
    WiFi.begin(WLAN_SSID, WLAN_SECRET);
    log_print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(1000);
        log_print(".");
    }

    // Print ESP8266 Local IP Address
    log_println();
    log_print("Connected: ");
    log_println(WiFi.localIP());
    
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", String("").c_str());
    });

    server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "application/json", String("{ \"state\": \"" + SystemStateToMessage(state) + "\" }").c_str());
    });

    server.on("/api/temperature", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "application/json", String("{ \"temperature\": { \"value\": \"" + String(temp) + "\", \"unit\": \"°C\" } }").c_str());
    });

    server.on("/api/humidity", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "application/json", String("{ \"humidity\": { \"value\": \"" + String(hum) + "\", \"unit\": \"%\" } }").c_str());
    });

    // Start server
    server.begin();
}
#endif

void isr_rpm_1()
{
    counter_rpm[RPM_1]++;
}

void isr_rpm_2()
{
    counter_rpm[RPM_2]++;
}