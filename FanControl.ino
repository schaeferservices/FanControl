/*
    Name:       FanControl
    Version:    2
    Created:	08/2020
    Author:     Daniel Schäfer
*/

/**************************************************/
/*                  Project settings              */
/**************************************************/
/**/        #define DEBUG                       /**/
/**/        #define TEST_SPEAKER 0              /**/
/**/        #define SERIAL_BAUD_RATE 115200     /**/
/**/        #define MILLIS_SLEEP_LOOP 1000      /**/
/**************************************************/

#include <DHT.h>
#include "PinSetup.h"
#include "AdditionalDefinitions.h"

DHT dht(PIN_DHT11_DATA, DHT11);

int counter_seconds = 0;
uint16_t counter_rpm[2] = { 0 }, rpm[2] = { 0 }, rpm_set = 0;
float avgTemp = 0;
ALARM_TYPE alarm = ALARM_OFF;
float temp = 0;
uint64_t previousMillis = 0;

void setup()
{
#ifdef DEBUG
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
}

void _main()
{
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

#ifdef DEBUG
    Serial.print("Temp: ");
    Serial.print(temp);
    Serial.print(" Hum: ");
    Serial.print(hum);
    Serial.print("%");
#endif

    // calc rpm
    calc_rpm(RPM_1);
    calc_rpm(RPM_2);
    Serial.println();

    if (counter_seconds == 0)
    {
        avgTemp = temp;
    }

    counter_seconds++;
    avgTemp = avgTemp + temp;

    if (counter_seconds >= 10)
    {
        avgTemp = avgTemp / 11;

#ifdef DEBUG
        Serial.print("avgTemp: ");
        Serial.println(avgTemp);
#endif

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
            alarm = ALARM_ON;
        }
        else
        {
            rpm_set = 0;
            analogWrite(PIN_PWM_1, rpm_set);
            analogWrite(PIN_PWM_2, rpm_set);
        }

#ifdef DEBUG
        Serial.print("RPM_set: ");
        Serial.print(rpm_set / 2.55);
        Serial.println("%");
#endif

        counter_seconds = 0;
    }
    else if (isnan(temp))
    {
        rpm_set = 255;
        analogWrite(PIN_PWM_1, rpm_set);
        analogWrite(PIN_PWM_2, rpm_set);
        alarm = ALARM_ON;

#ifdef DEBUG
        Serial.println();
        Serial.print("RPM_set: ");
        Serial.print(rpm_set / 2.55);
        Serial.println("%");
#endif
    }

    // set alarm state
    if (alarm == ALARM_ON)
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
}

void calc_rpm(uint8_t num)
{
    rpm_interrupts_disable();

    rpm[num] = counter_rpm[num] * (60 / 2);

#ifdef DEBUG
    Serial.print(" RPM_FAN_");
    Serial.print(num);
    Serial.print(": ");
    Serial.print(rpm[num] / 18);
    Serial.print("% ");
#endif

    if ((abs((rpm_set / 2.55) - (rpm[num] / 18)) > 10) || TEST_SPEAKER)
    {
        alarm = ALARM_ON;
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

#ifdef NOTDEF
void initWebApi()
{
    // https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.println("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println(".");
    }

    // Print ESP8266 Local IP Address
    Serial.println(WiFi.localIP());

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", index_html, processor);
        });
    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/plain", String(t).c_str());
        });
    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/plain", String(h).c_str());
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