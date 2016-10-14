#include "utility.h"

void CheckHeapMemory() {
  app_log(APP_LOG_LEVEL_INFO, __FILE_NAME__, __LINE__, 
      "FREE MEMORY - HEAP BYTES %u", heap_bytes_free());
}

void MenuCellDrawHeader(GContext* ctx, 
                        const Layer *cell_layer,
                        const char* text) {
  GSize size = layer_get_frame(cell_layer).size;
  graphics_draw_text(ctx, 
                     text,
                     fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(0,0,size.w,size.h),
                     GTextOverflowModeTrailingEllipsis,
                     PBL_IF_ROUND_ELSE(GTextAlignmentCenter,
                                       GTextAlignmentLeft), 
                     NULL);
}

void MenuCellDraw(GContext *ctx, const Layer *cell_layer, 
  const char* title, const char* details) {
  GRect bounds = layer_get_bounds(cell_layer);

  GFont title_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  GFont details_font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  
  uint y_offset = -4;
  uint padding = 4;
  
  // title_layer
  int edge_padding = PBL_IF_ROUND_ELSE(
      menu_cell_layer_is_highlighted(cell_layer) ? 12: 24, 4);
  int title_width = bounds.size.w - edge_padding*2;
  int title_height = bounds.size.h/3;
  
  GRect title_bounds = GRect(edge_padding, y_offset, title_width, title_height);

  graphics_draw_text(ctx, 
                     title,
                     title_font,
                     title_bounds,
                     GTextOverflowModeTrailingEllipsis,
                     PBL_IF_ROUND_ELSE(GTextAlignmentCenter, 
                                       GTextAlignmentLeft), 
                     NULL);

  // details
  int details_width = title_width;
  int details_height = bounds.size.h - title_height - padding;
  GRect details_bounds = GRect(edge_padding, 
                               title_height + y_offset + padding, 
                               details_width, 
                               details_height);
  
#ifdef PBL_ROUND
  if(menu_cell_layer_is_highlighted(cell_layer)) {
#endif
  graphics_draw_text(ctx, 
                     details, 
                     details_font, 
                     details_bounds,
                     GTextOverflowModeTrailingEllipsis,
                     PBL_IF_ROUND_ELSE(GTextAlignmentCenter,
                                       GTextAlignmentLeft), 
                     NULL);
#ifdef PBL_ROUND
  }
#endif
}

void StringCopy(char* a, const char* b, uint s) {
  if(s >= 1) {
    strncpy(a, b, s);
    a[s-1] = '\0';;
  }
}

bool StringAllocateAndCopy(char** a, const char* b) {
  int i = strlen(b);
  *a = (char *)malloc(sizeof(char)*(i+1));
  if(*a != NULL) {
    memcpy(*a, b, i+1);
    return true;
  }
  else {
    return false;
  }
}

void FreeAndClearPointer(void** ptr) {
  free(*ptr);
  *ptr = NULL;
}
