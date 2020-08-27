#include "AdditionalDefinitions.h"

#ifdef ESP8266

#include <ESP8266WiFi.h>

#endif

String ComponentStateToMessage(ComponentState state)
{
	switch (state)
	{
		case STATE_OK:
			return "OK";

		case STATE_ERROR:
			return "ERROR";

		default:
			return "ERROR_UNKOWN";
	}
}

ComponentState GetSystemState(ComponentState state[])
{
	for (int i = 0; i < COUNT_COMPONENT_STATES; i++)
	{
		if (state[i] != STATE_OK)
		{
			return STATE_ERROR;
		}
	}

	return STATE_OK;
}

void log_print_state(ComponentState state[])
{
	String str = "FAN_1: " + ComponentStateToMessage(state[FAN_1]) + "\n"
			   + "FAN_2: " + ComponentStateToMessage(state[FAN_2]) + "\n"
			   + "DHT_TEMP: " + ComponentStateToMessage(state[DHT_TEMP]) + "\n"
			   + "DHT_HUM: " + ComponentStateToMessage(state[DHT_HUM]) + "\n";

	log_print(str);
}

float TempToByte(float temp, int fanNum)
{
	int fan_min_rpm = fanNum == FAN_1 ? FAN_1_MIN_RPM : (fanNum == FAN_2 ? FAN_2_MIN_RPM : NULL);
	int fan_max_rpm = fanNum == FAN_1 ? FAN_1_MAX_RPM : (fanNum == FAN_2 ? FAN_2_MAX_RPM : NULL);

	if (temp < 20)
	{
		return 0;
	}
	else if (temp > 50)
	{
		return 255;
	}

	float res = ((temp - 1) * (50 - 20) / (255.0 - 1) + 20) + ((float)fan_min_rpm / fan_max_rpm) * 255;

	return res > 255 ? 255 : res;
}

float RPMToPercent(uint16_t rpm, int fanNum)
{
	int fan_max_rpm = fanNum == FAN_1 ? FAN_1_MAX_RPM : (fanNum == FAN_2 ? FAN_2_MAX_RPM : NULL);
	int fan_min_rpm = fanNum == FAN_1 ? FAN_1_MIN_RPM : (fanNum == FAN_2 ? FAN_2_MIN_RPM : NULL);

	if (rpm < fan_min_rpm)
	{
		return 0;
	}
	else if (rpm > fan_max_rpm)
	{
		return 100;
	}

	return (rpm - fan_min_rpm) * (100 - 1) / ((float)fan_max_rpm - fan_min_rpm) + 1;
}

void log_print(char* str)
{
#ifdef _DEBUG
	Serial.print(str);
#endif
}

void log_print(int i)
{
#ifdef _DEBUG
	Serial.print(i);
#endif
}

void log_println(char* str)
{
#ifdef _DEBUG
	Serial.println(str);
#endif
}

void log_println(int i)
{
#ifdef _DEBUG
	Serial.println(i);
#endif
}

void log_print(String str)
{
#ifdef _DEBUG
	Serial.print(str.c_str());
#endif
}

void log_println(String str)
{
#ifdef _DEBUG
	Serial.println(str.c_str());
#endif
}

#ifdef ESP8266

void log_print(IPAddress ip)
{
#ifdef _DEBUG
	Serial.print(ip);
#endif
}

void log_println(IPAddress ip)
{
#ifdef _DEBUG
	Serial.println(ip);
#endif
}

#endif ESP8266