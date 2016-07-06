#ifndef ERROR_WINDOW_H
#define ERROR_WINDOW_H

#include <pebble.h>

#define DIALOG_MESSAGE_WINDOW_MARGIN 5

void ErrorWindowPush(const char *, bool);
void ErrorWindowRemove();

#endif //ERROR_WINDOW_H