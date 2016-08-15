#ifndef UTILITY_H
#define UTILITY_H

#include <pebble.h>

#ifdef RELEASE
#undef APP_LOG
#define APP_LOG(...)
#endif

void MenuCellDrawHeader(GContext* ctx, 
                        const Layer *cell_layer,
                        const char* text);
void MenuCellDraw(GContext *ctx, 
                  const Layer *cell_layer, 
                  const char* title, 
                  const char* details);
void StringCopy(char* a, const char* b, uint s);
void StringAllocateAndCopy(char** a, const char* b);
void FreeAndClearPointer(void** ptr);

#endif /* end of include guard: UTILITY_H */
