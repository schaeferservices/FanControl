#include "AdditionalDefinitions.h"

String SystemStateToMessage(SystemState state)
{
	switch (state)
	{
		case STATUS_OK:
			return "OK";
			break;

		case ERROR:
			return "ERROR";
			break;
		
		case ERROR_TEMP:
			return "ERROR_TEMP";
			break;

		case ERROR_RPM:
			return "ERROR_RPM";
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