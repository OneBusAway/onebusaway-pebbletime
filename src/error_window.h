#ifndef ERROR_WINDOW_H
#define ERROR_WINDOW_H

#include <pebble.h>

#define DIALOG_MESSAGE_WINDOW_MARGIN 5
#define MAX_ERROR_STRING_LENGTH 256

void ErrorWindowInit();
void ErrorWindowDeinit();
void ErrorWindowPush(const char *, bool);
void ErrorWindowRemove();

#endif //ERROR_WINDOW_H