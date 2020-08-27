/*
    Name:       FanControl
    Version:    2.6
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
#include "FanSetup.h"
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
uint16_t counter_rpm[2] = { 0 }, rpm[2] = { 0 }, rpm_set[2] = { 0 };
float avgTemp = 0, temp = 0, hum = 0;
ComponentState prevState = STATE_OK;
uint64_t previousMillis = 0;
ComponentState state[COUNT_COMPONENT_STATES] = { STATE_OK, STATE_OK, STATE_OK, STATE_OK };
ComponentState state_hard[COUNT_COMPONENT_STATES] = { STATE_OK, STATE_OK, STATE_OK, STATE_OK };
int state_count_hardstate_ok[COUNT_COMPONENT_STATES] = { 0 }, state_count_hardstate_error[COUNT_COMPONENT_STATES] = { 0 };
unsigned long prevMillisRPM[2] = { 0 };

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
    calc_rpm(FAN_1);
    calc_rpm(FAN_2);
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

        if (avgTemp >= 20)
        {
            rpm_set[FAN_1] = TempToByte(avgTemp, FAN_1);
            rpm_set[FAN_2] = TempToByte(avgTemp, FAN_2);

            state[DHT_TEMP] = STATE_OK;
        }
        else if (isnan(avgTemp))
        {
            rpm_set[FAN_1] = 255;
            rpm_set[FAN_2] = 255;

            state[DHT_TEMP] = STATE_ERROR;
        }
        else
        {
            rpm_set[FAN_1] = 0;
            rpm_set[FAN_2] = 0;

            state[DHT_TEMP] = STATE_OK;
        }

        analogWrite(PIN_PWM_1, rpm_set[FAN_1]);
        analogWrite(PIN_PWM_2, rpm_set[FAN_2]);

        log_println("\n#SET-RPM_FAN_1: " + String(rpm_set[FAN_1] / 2.55) + "%" + "\n#SET-RPM_FAN_2: " + String(rpm_set[FAN_2] / 2.55) + "%");

        counter_seconds = 0;
    }
    else if (isnan(temp))
    {
        rpm_set[FAN_1] = 255;
        rpm_set[FAN_2] = 255;

        analogWrite(PIN_PWM_1, rpm_set[FAN_1]);
        analogWrite(PIN_PWM_2, rpm_set[FAN_2]);
        state[DHT_TEMP] = STATE_ERROR;

        log_println("\n#SET-RPM_FAN_1: " + String(rpm_set[FAN_1] / 2.55) + "%" + "\n#SET-RPM_FAN_2: " + String(rpm_set[FAN_2] / 2.55) + "%");
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

    rpm[num] = (counter_rpm[num] / ((millis() - prevMillisRPM[num]) / 1000.0)) * 60;
    log_print("\n## rpm: " + String(rpm[num]) + "\n");
    float rpm_percent = RPMToPercent(rpm[num], num == FAN_1 ? FAN_1 : (num == FAN_2 ? FAN_2 : UNDEFINED));

    log_print(" RPM_FAN_" + String(num) + ": " + String(rpm_percent) + "% ");

    if (abs((rpm_set[num] / 2.55) - rpm_percent) > 20 || (rpm_percent <= 2 && (rpm_set[num] / 2.55) > 2))
    {
        state[num == FAN_1 ? FAN_1 : (num == FAN_2 ? FAN_2 : UNDEFINED)] = STATE_ERROR;
    }
    else
    {
        state[num == FAN_1 ? FAN_1 : (num == FAN_2 ? FAN_2 : UNDEFINED)] = STATE_OK;
    }

    counter_rpm[num] = 0;
    prevMillisRPM[num] = millis();

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
        request->send_P(200, "application/json", String("{ \"state\": \"" + ComponentStateToMessage(GetSystemState(state_hard)) + "\" }").c_str());
    });

    server.on("/api/state/components/FAN_1", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "application/json", String("{ \"state\": \"" + ComponentStateToMessage(state_hard[FAN_1]) + "\" }").c_str());
    });

    server.on("/api/state/components/FAN_2", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "application/json", String("{ \"state\": \"" + ComponentStateToMessage(state_hard[FAN_2]) + "\" }").c_str());
    });

    server.on("/api/state/components/DHT_TEMP", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "application/json", String("{ \"state\": \"" + ComponentStateToMessage(state_hard[DHT_TEMP]) + "\" }").c_str());
    });

    server.on("/api/state/components/DHT_HUM", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "application/json", String("{ \"state\": \"" + ComponentStateToMessage(state_hard[DHT_HUM]) + "\" }").c_str());
    });

    server.on("/api/state/components", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "application/json", String("{ \"components\": [ \"FAN_1\", \"FAN_2\", \"DHT_TEMP\", \"DHT_HUM\" ] }").c_str());
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
    counter_rpm[FAN_1]++;
}

void isr_rpm_2()
{
    counter_rpm[FAN_2]++;
}