#include "AdditionalDefinitions.h"

String SystemStateToMessage(SystemState state)
{
	switch (state)
	{
		case STATUS_OK:
			return String("OK");
			break;

		case ERROR:
			return String("ERROR");
			break;
		
		case ERROR_TEMP:
			return String("ERROR_TEMP");
			break;

		case ERROR_RPM:
			return String("ERROR_RPM");
			break;
		default:
			return String("ERROR_UNKNOWN");
			break;
	}
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