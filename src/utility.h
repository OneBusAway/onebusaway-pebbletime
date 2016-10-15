#ifndef UTILITY_H
#define UTILITY_H

#include <pebble.h>

#ifndef LOGGING_ENABLED
#undef APP_LOG
#define APP_LOG(...)
#endif

#define MAX(x,y) (x > y ? x : y)
#define MIN(x,y) (x > y ? y : x)

void CheckHeapMemory();
void MenuCellDrawHeader(GContext* ctx, 
                        const Layer *cell_layer,
                        const char* text);
void MenuCellDraw(GContext *ctx, 
                  const Layer *cell_layer, 
                  const char* title, 
                  const char* details);
void StringCopy(char* a, const char* b, uint s);
bool StringAllocateAndCopy(char** a, const char* b);
void FreeAndClearPointer(void** ptr);

#endif /* end of include guard: UTILITY_H */
