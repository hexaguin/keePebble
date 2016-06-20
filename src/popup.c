#include "pebble.h"
#include "globals.h"
//Most of this code is from Pebble's UI demo
static Window *s_main_window;
static TextLayer *s_label_layer;
static Layer *s_background_layer, *s_icon_layer;

static void background_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite));
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, 0);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  const GEdgeInsets background_insets = {.top = bounds.size.h  /* Start hidden */};
  s_background_layer = layer_create(grect_inset(bounds, background_insets));
  layer_set_update_proc(s_background_layer, background_update_proc);
  layer_add_child(window_layer, s_background_layer);

  s_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WARNING);
  GRect bitmap_bounds = gbitmap_get_bounds(s_icon_bitmap);

  s_icon_layer = layer_create(PBL_IF_ROUND_ELSE(
      GRect((bounds.size.w - bitmap_bounds.size.w) / 2, bounds.size.h + DIALOG_MESSAGE_WINDOW_MARGIN, bitmap_bounds.size.w, bitmap_bounds.size.h),
      GRect(DIALOG_MESSAGE_WINDOW_MARGIN, bounds.size.h + DIALOG_MESSAGE_WINDOW_MARGIN, bitmap_bounds.size.w, bitmap_bounds.size.h)
  ));
  layer_set_update_proc(s_icon_layer, icon_update_proc);
  layer_add_child(window_layer, s_icon_layer);

  s_label_layer = text_layer_create(GRect(DIALOG_MESSAGE_WINDOW_MARGIN, bounds.size.h + DIALOG_MESSAGE_WINDOW_MARGIN + bitmap_bounds.size.h, bounds.size.w - (2 * DIALOG_MESSAGE_WINDOW_MARGIN), bounds.size.h));
  text_layer_set_text(s_label_layer, DIALOG_MESSAGE_WINDOW_MESSAGE);
  text_layer_set_background_color(s_label_layer, GColorClear);
  text_layer_set_text_alignment(s_label_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft));
  text_layer_set_font(s_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_label_layer));
}

static void window_unload(Window *window) {
  layer_destroy(s_background_layer);
  layer_destroy(s_icon_layer);

  text_layer_destroy(s_label_layer);

  gbitmap_destroy(s_icon_bitmap);

  window_destroy(window);
  s_main_window = NULL;
}

void popup_window_push() {
  if(!s_main_window) {
    s_main_window = window_create();
    window_set_background_color(s_main_window, GColorBlack);
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });
  }
  window_stack_push(s_main_window, true);
}
