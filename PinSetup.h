#pragma once

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