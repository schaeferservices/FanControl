#pragma once

#define RPM_1 0
#define RPM_2 1

#define COUNT_COMPONENT_STATES 5
#define HARDSTATE_VALIDATION_AMMOUNT 5

#if DEBUG
#define _DEBUG
#endif

enum ComponentState { STATE_OK, STATE_ERROR };
enum Component { UNDEFINED, FAN_1, FAN_2, DHT_TEMP, DHT_HUM };

String ComponentStateToMessage(ComponentState state);
ComponentState GetSystemState(ComponentState state[]);
void log_print_state(ComponentState state[]);
void log_print(char* str);
void log_println(char* str = "");
void log_print(int i);
void log_println(int i);
void log_print(String str);
void log_println(String str);

#ifdef ESP8266

void log_print(IPAddress ip);
void log_println(IPAddress ip);

#endif