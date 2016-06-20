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

enum HandshakeStatus{
  no_handshake, //No handshaking has happened yet
  waiting_for_version, //Waiting for the phone to respond with a system packet
  handshake_complete, //Succesfully did handshake, everything is ready to go
  handshake_error //Handshake failed, most likely due to mismatched versions
};

enum HandshakeStatus currentStatus = no_handshake;

void sendSelection(){
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);

	dict_write_uint8(iterator, 0, 2);
	dict_write_uint8(iterator, 1, pickedItem);

	app_message_outbox_send();

	app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
	app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
}

void received_data(DictionaryIterator *received, void *context) { //Inbound packets
	uint8_t module = dict_find(received, 0)->value->uint8;
  uint8_t id = dict_find(received, 1)->value->uint8;
  
  switch(module) {
    case 0: //System module
      switch(id) {
        case 0: //Protocol version
          if( (dict_find(received, 2)->value->uint16) == PROTOCOL_VERSION ){ //if recieved version matches the current used version
            currentStatus = handshake_complete;
            APP_LOG(APP_LOG_LEVEL_DEBUG, "Handshake complete, using protocol version %u", PROTOCOL_VERSION);
          } else {
            currentStatus = handshake_error; //TODO error screen
            APP_LOG(APP_LOG_LEVEL_ERROR, "Handshake failed, watch is running version %u, phone is running %u", PROTOCOL_VERSION, dict_find(received, 2)->value->uint16);
          }
          break; //end of protocol version
        
        case 1: //Error packet
          switch(dict_find(received, 2)->value->uint16){
            case 0: //Protocol mismatch
              APP_LOG(APP_LOG_LEVEL_ERROR, "Protocol missmatch detected by phone");
              currentStatus = handshake_error;
              break; //End of mismatch
            default:
              APP_LOG(APP_LOG_LEVEL_ERROR, "Unknown error packet: %u", dict_find(received, 2)->value->uint16); //print the unknown error code
          }
          break; //End of error packet
          
        default: //unkown system packet
          APP_LOG(APP_LOG_LEVEL_WARNING, "Unknown system packet id: %u", id);
      }
      break;
    
    default: //unknown module #
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Unknown module: %u", module);
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
	app_message_open(256, 64);
  
  s_main_window = window_create(); //Make the main window
  window_set_window_handlers(s_main_window, (WindowHandlers) { //Add handlers
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_stack_push(s_main_window, true);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Windows made, sending protocol version");
  
  psleep(500); //HACK REMOVE THIS
  
  app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED); //Do phone handshake
	DictionaryIterator *iterator;
	app_message_outbox_begin(&iterator);
	dict_write_uint8(iterator, 0, 0); //Set module id to 0 (system)
  dict_write_uint8(iterator, 1, 0); //Set packet id to 0 (version)
  dict_write_uint16(iterator, 2, PROTOCOL_VERSION); //Set packet contents to protocol version
	app_message_outbox_send();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Sent protocol version: %u", PROTOCOL_VERSION);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}