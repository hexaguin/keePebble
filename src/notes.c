#include "pebble.h"
#include "pebble_fonts.h"
#include "globals.h"

char fullNote[3000];
TextLayer* fullNoteText;
ScrollLayer* scroller;
StatusBarLayer* s_statusbar_layer;
Window* fullWindow;

void note_data_received(DictionaryIterator* iterator) {
	uint16_t location = dict_find(iterator, 1)->value->uint16;
	uint16_t segmentLength = dict_find(iterator, 2)->value->uint16;

	memcpy((void *) &fullNote[location], dict_find(iterator, 4)->value->cstring, segmentLength);
	GSize maxSize = graphics_text_layout_get_content_size(fullNote, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, 0, 144, 1000), GTextOverflowModeWordWrap, GTextAlignmentLeft);
	if (maxSize.h < 168 - 16)
		maxSize.h = 168 - 16;

	text_layer_set_size(fullNoteText, maxSize);
	scroll_layer_set_content_size(scroller, maxSize);


	text_layer_set_text(fullNoteText, fullNote);

	if (segmentLength == 75) {
		DictionaryIterator *iterator;
		app_message_outbox_begin(&iterator);

		dict_write_uint8(iterator, 0, 3);
		dict_write_uint16(iterator, 1, location + 75);

		app_message_outbox_send();

		app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
		app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
	}
}

static void note_window_load(Window *window) {
  //TODO populate this with all the windowy stuff currently in note_init
}

static void note_window_unload(Window *window) {
  displayingNote = false;
}

void note_init() { 
	fullWindow = window_create();
	Layer* topLayer = window_get_root_layer(fullWindow);
  GRect windowBounds = layer_get_bounds(topLayer);
  
  window_set_window_handlers(fullWindow, (WindowHandlers) {
    .load = note_window_load,
    .unload = note_window_unload
  });
  
  s_statusbar_layer = status_bar_layer_create();
  status_bar_layer_set_colors(s_statusbar_layer, COLOR_HIGHLIGHT, COLOR_FOREGROUND);
  status_bar_layer_set_separator_mode(s_statusbar_layer, StatusBarLayerSeparatorModeDotted);
  
  windowBounds.origin.y+=STATUS_BAR_LAYER_HEIGHT; //resize "usable" area to not overlap with statusbar
  windowBounds.size.h-=STATUS_BAR_LAYER_HEIGHT;

	fullNoteText = text_layer_create(windowBounds);
	text_layer_set_font(fullNoteText, fonts_get_system_font(FONT_KEY_GOTHIC_18));

	scroller = scroll_layer_create(windowBounds);
  scroll_layer_set_shadow_hidden(scroller, true);
	scroll_layer_set_click_config_onto_window(scroller, fullWindow);
	scroll_layer_add_child(scroller, (Layer *) fullNoteText);

	for (uint16_t i = 0; i < 3000; i++)
		fullNote[i] = 0;

  layer_add_child(topLayer, status_bar_layer_get_layer(s_statusbar_layer));
	layer_add_child(topLayer, (Layer *) scroller);

	window_stack_push(fullWindow, true /* Animated */);
}
