#include <pebble.h>
#include "notes.h"
#include "globals.h"

bool displayingNote = false;

static Window *s_main_window;
MenuLayer *s_menu_layer;
TextLayer *s_loading_layer;
StatusBarLayer *s_statusbar_layer;

char items[50][20]; //Array of note items
uint8_t numOfItems = 0;

bool loading = true;
int8_t pickedItem = -1;

extern bool displayingNote;

void sendSelection(){
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);

	dict_write_uint8(iterator, 0, 2);
	dict_write_uint8(iterator, 1, pickedItem);

	app_message_outbox_send();

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
}

void list_part_received(DictionaryIterator *received){ //TODO figure out how this whole blob of code works
	uint8_t index = dict_find(received, 1)->value->uint8;
	numOfItems = dict_find(received, 2)->value->uint8;

	bool listFinished = false;

	for (uint8_t i = 0; i < 3; i++) {
		uint8_t listPos = index + i;
    
		if (listPos > numOfItems - 1) {
			listFinished = true;
			break;
		}
		strcpy(items[listPos], dict_find(received, i + 3)->value->cstring);
	}

	if (index + 3 == numOfItems) listFinished = true;

	if (listFinished)	{
		loading = false;
	}
	else {
		if (pickedItem != -1)	{
			sendSelection();
			return;
		}

		DictionaryIterator *iterator;
		app_message_outbox_begin(&iterator);

		dict_write_uint8(iterator, 0, 1);
		dict_write_uint8(iterator, 1, index + 3);

		app_message_outbox_send();

		app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
		app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
	}
  
	layer_set_hidden((Layer*) s_loading_layer, true);
	layer_set_hidden((Layer*) s_menu_layer, false);

	menu_layer_reload_data(s_menu_layer);
}

void received_data(DictionaryIterator *received, void *context) {
	if (displayingNote)	{
		note_data_received(received);
		return;
	}

	uint8_t id = dict_find(received, 0)->value->uint8;

	switch (id)	{
  	case 0:
  		list_part_received(received);
  		break;
  	case 1:
  		displayingNote = true;
  		note_init();
  		note_data_received(received);
  		break;
	}
}

uint16_t get_num_rows_callback(MenuLayer *me, uint16_t section_index, void *data) {
	return numOfItems;
}

static void draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  menu_cell_basic_draw(ctx, cell_layer, items[cell_index->row], NULL, NULL);
}

static int16_t get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return PBL_IF_ROUND_ELSE(
    menu_layer_is_index_selected(menu_layer, cell_index) ?
      MENU_CELL_ROUND_FOCUSED_SHORT_CELL_HEIGHT : MENU_CELL_ROUND_UNFOCUSED_TALL_CELL_HEIGHT,
    44);
}

static void select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  pickedItem = cell_index->row;

	if (!loading)
		sendSelection();
}

static void main_window_load(Window *window) {
  
  Layer *window_layer = window_get_root_layer(window);
  s_statusbar_layer = status_bar_layer_create();
  status_bar_layer_set_colors(s_statusbar_layer, COLOR_HIGHLIGHT, COLOR_FOREGROUND);
  status_bar_layer_set_separator_mode(s_statusbar_layer, StatusBarLayerSeparatorModeDotted);
  
  GRect bounds = layer_get_bounds(window_layer);
  bounds.origin.y+=STATUS_BAR_LAYER_HEIGHT; //resize "usable" area to not overlap with statusbar
  bounds.size.h-=STATUS_BAR_LAYER_HEIGHT;
  
  GRect loadingBounds = GRect(0, 84, 144, 84 - 16);
  s_loading_layer = text_layer_create(loadingBounds);
	text_layer_set_font(s_loading_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
	text_layer_set_text(s_loading_layer, "Loading...");
  text_layer_set_text_alignment(s_loading_layer, GTextAlignmentCenter);
  
  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
#if defined(PBL_COLOR)
  menu_layer_set_normal_colors(s_menu_layer, COLOR_BACKGROUND, COLOR_FOREGROUND);
  menu_layer_set_highlight_colors(s_menu_layer, COLOR_HIGHLIGHT, COLOR_FOREGROUND);
#endif
  
  layer_add_child(window_layer, (Layer*) s_loading_layer);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
      .get_num_rows = get_num_rows_callback,
      .draw_row = draw_row_callback,
      .get_cell_height = get_cell_height_callback,
      .select_click = select_callback,
  });
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  layer_add_child(window_layer, status_bar_layer_get_layer(s_statusbar_layer));
  
  layer_set_hidden((Layer*) s_loading_layer, false);
	layer_set_hidden((Layer*) s_menu_layer, true);
}

static void main_window_unload(Window *window) {
  
}

static void init() {
  app_message_register_inbox_received(received_data);
	app_message_open(120, 20);
  
  s_main_window = window_create(); //Make the main window
  window_set_window_handlers(s_main_window, (WindowHandlers) { //Add handlers
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_stack_push(s_main_window, true);
  
  app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED); //Do phone magic
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 0);
	app_message_outbox_send();
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}