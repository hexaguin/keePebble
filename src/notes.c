#include "pebble.h"
#include "pebble_fonts.h"

char fullNote[3000];
TextLayer* fullNoteText;
ScrollLayer* scroller;
Window* fullWindow;

void note_data_received(DictionaryIterator* iterator)
{
	uint16_t location = dict_find(iterator, 1)->value->uint16;
	uint16_t segmentLength = dict_find(iterator, 2)->value->uint16;

	memcpy((void *) &fullNote[location], dict_find(iterator, 4)->value->cstring, segmentLength);
	GSize maxSize = graphics_text_layout_get_content_size(fullNote, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, 0, 144, 1000), GTextOverflowModeWordWrap, GTextAlignmentLeft);
	if (maxSize.h < 168 - 16)
		maxSize.h = 168 - 16;

	text_layer_set_size(fullNoteText, maxSize);
	scroll_layer_set_content_size(scroller, maxSize);


	text_layer_set_text(fullNoteText, fullNote);

	if (segmentLength == 75)
	{
		DictionaryIterator *iterator;
		app_message_outbox_begin(&iterator);

		dict_write_uint8(iterator, 0, 3);
		dict_write_uint16(iterator, 1, location + 75);

		app_message_outbox_send();

		app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
		app_comm_set_sniff_interval(SNIFF_INTERVAL_NORMAL);
	}

}

void note_init()
{
	GRect windowBounds = GRect(0, 0, 144, 168 - 16);

	fullWindow = window_create();
	Layer* topLayer = window_get_root_layer(fullWindow);

	fullNoteText = text_layer_create(windowBounds);
	text_layer_set_font(fullNoteText, fonts_get_system_font(FONT_KEY_GOTHIC_18));

	scroller = scroll_layer_create(windowBounds);
	scroll_layer_set_click_config_onto_window(scroller, fullWindow);
	scroll_layer_add_child(scroller, (Layer *) fullNoteText);

	for (uint16_t i = 0; i < 3000; i++)
		fullNote[i] = 0;

	layer_add_child(topLayer, (Layer *) scroller);

	window_stack_push(fullWindow, true /* Animated */);
}
