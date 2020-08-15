#pragma once

#define RPM_1 0
#define RPM_2 1

#if DEBUG
#define _DEBUG
#endif

enum SystemState { STATUS_OK, ERROR, ERROR_TEMP, ERROR_RPM };

String SystemStateToMessage(SystemState state);
void log_print(char* str);
void log_println(char* str = "");
void log_print(int i);
void log_println(int i);