/*
    Name:       FanControl
    Version:    2
    Created:	14.08.2020 17:20:40
    Author:     Daniel Schäfer
*/

#include <Adafruit_Sensor.h>
#include <DHT_U.h>

#define DEBUG
#define TEST_SPEAKER 0


#ifdef __AVR_ATmega328P__

#define ATMEGA328P
#define PIN_DHT11_DATA 6
#define PIN_PWM_1 10
#define PIN_PWM_2 11
#define PIN_RPM_1 2
#define PIN_RPM_2 3
#define PIN_SPEAKER 9

#else

#define ESP8266
#define PIN_DHT11_DATA D5
#define PIN_PWM_1 D2
#define PIN_PWM_2 D6
#define PIN_RPM_1 digitalPinToInterrupt(D1)
#define PIN_RPM_2 digitalPinToInterrupt(D8)
#define PIN_SPEAKER D7

#endif

#define RPM_1 0
#define RPM_2 1

DHT_Unified dht(PIN_DHT11_DATA, DHT11);

int rpm_count_1 = 0, rpm_count_2 = 0;
uint16_t counter_rpm[2] = { 0 }, rpm[2] = { 0 }, rpm_set[2] = { 0 };

void setup()
{
    Serial.begin(9600);
    dht.begin();

    pinMode(PIN_PWM_1, OUTPUT);
    pinMode(PIN_PWM_2, OUTPUT);
    pinMode(PIN_SPEAKER, OUTPUT);
    pinMode(PIN_DHT11_DATA, INPUT);
    pinMode(PIN_RPM_1, INPUT);
    pinMode(PIN_RPM_2, INPUT);

    //digitalWrite(PIN_RPM_1, HIGH);
    //digitalWrite(PIN_RPM_2, HIGH);
    attachInterrupt(PIN_RPM_1, isr_rpm_1, FALLING);
    attachInterrupt(PIN_RPM_2, isr_rpm_2, FALLING);

#ifdef ESP8266

    timer1_attachInterrupt(isr_loop);
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    timer1_write(1000000);

#endif
#ifdef ATMEGA328P

    // configure Timer 1 as "CTC mode"
    TCCR1A = TCCR1A & ~(1 << WGM11);
    TCCR1B = TCCR1B | (1 << WGM12);
    TCCR1A = TCCR1A & ~(1 << WGM10);

    // set prescaler to 1024
    TCCR1B = TCCR1B & ~(1 << CS11);
    TCCR1B = TCCR1B | (1 << CS12 | 1 << CS10);

    // define output compare value
    OCR1AH = 0x3D;
    OCR1AL = 0x09;

    // enable timer interrupt to 15625
    TIMSK1 = TIMSK1 | (1 << OCIE1A);

#endif
}

void isr_loop()
{
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    float temp = event.temperature;

#ifdef DEBUG
    Serial.print("Temp: ");
    Serial.println(temp);
#endif

    // calc rpm
    calc_rpm(RPM_1);
    calc_rpm(RPM_2);
}

void loop()
{

}

void calc_rpm(uint8_t num)
{
#ifdef ATMEGA328P
    cli();
#endif

    rpm[num] = counter_rpm[num] * (60 / 2);

#ifdef DEBUG
    Serial.print("RPM: ");
    Serial.print(rpm[num] / 18);
    Serial.print("% ");
#endif

    if ((abs((rpm_set[num] / 2.55) - (rpm[num] / 18)) > 10) || TEST_SPEAKER)
    {
        digitalWrite(PIN_SPEAKER, HIGH);
    }
    else
    {
        digitalWrite(PIN_SPEAKER, LOW);
    }

    counter_rpm[num] = 0;

#ifdef ESP8266
    timer1_write(1000000);
#endif
#ifdef ATMEGA328P
    sei();
#endif
}

void isr_rpm_1()
{
    counter_rpm[RPM_1]++;
}

void isr_rpm_2()
{
    counter_rpm[RPM_2]++;
}

#ifdef ATMEGA328P

ISR(TIMER1_COMPA_vect)
{
    isr_loop();
}

#endif