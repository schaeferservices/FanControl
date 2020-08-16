/*
    Name:       FanControl
    Version:    2.4
    Created:	08/2020
    Author:     Daniel Schäfer
*/

/**************************************************/
/**                 Project settings             **/
/**************************************************/
/**/     #define DEBUG 1                        /**/
/**/     #define TEST_SPEAKER 0                 /**/
/**/     #define SERIAL_BAUD_RATE 115200        /**/
/**/     #define MILLIS_SLEEP_LOOP 2000         /**/
/**/     #define OTA_ENABLE 1                   /**/
/**************************************************/

#include <Arduino.h>

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
float avgTemp = 0, temp = 0, hum = 0;
ComponentState prevState = STATE_OK;
uint64_t previousMillis = 0;
ComponentState state[COUNT_COMPONENT_STATES] = { STATE_OK, STATE_OK, STATE_OK, STATE_OK };
ComponentState state_hard[COUNT_COMPONENT_STATES] = { STATE_OK, STATE_OK, STATE_OK, STATE_OK };
int state_count_hardstate_ok[COUNT_COMPONENT_STATES] = { 0 }, state_count_hardstate_error[COUNT_COMPONENT_STATES] = { 0 };

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

    log_print("Temp: " + String(temp) + "°C Hum: " + String(hum) + "%");

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

        log_println("avgTemp: " + String(avgTemp) + "°C");

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
            state[DHT_TEMP] = STATE_OK;
        }
        else if (isnan(avgTemp))
        {
            rpm_set = 255;
            analogWrite(PIN_PWM_1, rpm_set);
            analogWrite(PIN_PWM_2, rpm_set);
            state[DHT_TEMP] = STATE_ERROR;
        }
        else
        {
            rpm_set = 0;
            analogWrite(PIN_PWM_1, rpm_set);
            analogWrite(PIN_PWM_2, rpm_set);
            state[DHT_TEMP] = STATE_OK;
        }

        log_println("\nRPM_set: " + String(rpm_set / 2.55) + "%");

        counter_seconds = 0;
    }
    else if (isnan(temp))
    {
        rpm_set = 255;
        analogWrite(PIN_PWM_1, rpm_set);
        analogWrite(PIN_PWM_2, rpm_set);
        state[DHT_TEMP] = STATE_ERROR;

        log_println("\nRPM_set: " + String(rpm_set / 2.55) + "%");
    }

    if (isnan(hum))
    {
        state[DHT_HUM] = STATE_ERROR;
    }
    else
    {
        state[DHT_HUM] = STATE_OK;
    }

    //state hardstate validation
    for (int i = 0; i < COUNT_COMPONENT_STATES; i++)
    {
        if (state[i] != STATE_OK)
        {
            state_count_hardstate_ok[i] = 0;
            state_count_hardstate_error[i]++;

            if (state_count_hardstate_error[i] >= HARDSTATE_VALIDATION_AMMOUNT)
            {
                state_count_hardstate_error[i] = 0;
                state_hard[i] = state[i];
            }
        }
        else
        {
            state_count_hardstate_error[i] = 0;
            state_count_hardstate_ok[i]++;

            if (state_count_hardstate_ok[i] >= HARDSTATE_VALIDATION_AMMOUNT)
            {
                state_count_hardstate_ok[i] = 0;
                state_hard[i] = state[i];
            }
        }
    }

    // state handling
    log_print_state(state_hard);
    if (GetSystemState(state_hard) != prevState)
    {
        if (GetSystemState(state_hard) != STATE_OK || TEST_SPEAKER)
        {
            digitalWrite(PIN_SPEAKER, HIGH);
            log_println("** ALARM ON **");
        }
        else
        {
            digitalWrite(PIN_SPEAKER, LOW);
            log_println("** ALARM OFF **");
        }

        prevState = GetSystemState(state_hard);
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

    log_print(" RPM_FAN_" + String(num) + ": " + String(rpm[num] / 18 < 100 ? rpm[num] / 18 : 100) + "% ");

    if (abs((rpm_set / 2.55) - (rpm[num] / 18)) > 20 || ((rpm[num] / 18) <= 2 && rpm_set > 2))
    {
        state[num == RPM_1 ? FAN_1 : (num == RPM_2 ? FAN_2 : UNDEFINED)] = STATE_ERROR;
    }
    else
    {
        state[num == RPM_1 ? FAN_1 : (num == RPM_2 ? FAN_2 : UNDEFINED)] = STATE_OK;
    }

    counter_rpm[num] = 0;

    rpm_interrupts_enable();
}

void rpm_interrupts_enable()
{
    attachInterrupt(INTERRUPT_RPM_1, isr_rpm_1, FALLING);
    attachInterrupt(INTERRUPT_RPM_2, isr_rpm_2, FALLING);
}

void rpm_interrupts_disable()
{
    detachInterrupt(INTERRUPT_RPM_1);
    detachInterrupt(INTERRUPT_RPM_2);
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
    log_print("\nConnected: ");
    log_println(WiFi.localIP());
    
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", String("").c_str());
    });

    server.on("/api/state/system", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "application/json", String("{ \"systemstate\": \"" + ComponentStateToMessage(GetSystemState(state)) + "\" }").c_str());
    });

    //TODO
    server.on("/api/state/components", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "application/json", String("{ \"systemstate\": \"" + ComponentStateToMessage(GetSystemState(state)) + "\" }").c_str());
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