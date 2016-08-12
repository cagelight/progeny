#ifndef PROGENY_COM_INTERNAL_H
#define PROGENY_COM_INTERNAL_H

#include "com.h"
#include "com_vk.h"

extern atomic bool com_close_semaphore;

extern xcb_connection_t * com_xcb_connection;
extern xcb_window_t com_xcb_window;
extern atomic bool xcb_close_semaphore;

bool com_xcb_init();
void com_xcb_term();
void com_xcb_frame();

bool com_vk_init();
void com_vk_term();

#endif//PROGENY_COM_INTERNAL_H
