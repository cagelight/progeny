#include "control.hpp"

std::atomic_bool control::run_sem {true};

static xcb_connection_t * com_xcb_connection = NULL;
static xcb_window_t com_xcb_window = 0;

static xcb_intern_atom_cookie_t delete_cookie;
static xcb_intern_atom_reply_t * delete_reply = NULL;

VkExtent2D current_extent {800, 600};

void control::init() {
	com_xcb_connection = xcb_connect( NULL, NULL );
	if ( !com_xcb_connection ) srcthrow("unable to open xcb connection");
	xcb_setup_t const * xcb_setup  = xcb_get_setup( com_xcb_connection );
	xcb_screen_iterator_t xcb_screen_iter   = xcb_setup_roots_iterator( xcb_setup );
	xcb_screen_t * xcb_screen = xcb_screen_iter.data;
	com_xcb_window = xcb_generate_id( com_xcb_connection );
	uint32_t value_list[] = { 0x00000000, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY };
	xcb_create_window( com_xcb_connection, XCB_COPY_FROM_PARENT, com_xcb_window, xcb_screen->root, 0, 0, current_extent.width, current_extent.height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, xcb_screen->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, value_list );
	
	xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom( com_xcb_connection, 1, 12, "WM_PROTOCOLS" );
	xcb_intern_atom_reply_t * protocols_reply = xcb_intern_atom_reply( com_xcb_connection, protocols_cookie, 0 );
	delete_cookie = xcb_intern_atom( com_xcb_connection, 0, 16, "WM_DELETE_WINDOW" );
	delete_reply = xcb_intern_atom_reply( com_xcb_connection, delete_cookie, 0 );
	xcb_change_property( com_xcb_connection, XCB_PROP_MODE_REPLACE, com_xcb_window, (*protocols_reply).atom, 4, 32, 1, &(*delete_reply).atom );
	free( protocols_reply );
	
	xcb_map_window(com_xcb_connection, com_xcb_window);
	xcb_flush(com_xcb_connection);
}

void control::term() noexcept {
	if (com_xcb_connection) {
		if (com_xcb_window) xcb_destroy_window(com_xcb_connection, com_xcb_window);
		xcb_disconnect(com_xcb_connection);
	}
}

void control::frame() {
	xcb_generic_event_t * event;
	
	handle_events:
	
	event = xcb_poll_for_event(com_xcb_connection);

	if (!event) return;
	
    switch (event->response_type & ~0x80) {
		case XCB_CONFIGURE_NOTIFY: {
			//xcb_expose_event_t * cev; //TODO - vulkan swapchain refresh on resize
			xcb_configure_notify_event_t * nev = (xcb_configure_notify_event_t *)event;
			if (nev->width != current_extent.width || nev->height != current_extent.height) {
				current_extent.width = nev->width;
				current_extent.height = nev->height;
				vk::swapchain::check_reinit();
			}
			break;
		}
		case XCB_CLIENT_MESSAGE: {
            if( (*(xcb_client_message_event_t*)event).data.data32[0] == (*delete_reply).atom ) {
				free( delete_reply );
				run_sem.store(false);
            }
			break;
		}
		case XCB_KEY_PRESS: {
			xcb_key_press_event_t * cev;
			cev = (xcb_key_press_event_t *)event;
			if (cev->detail == 9) run_sem.store(false);
			break;
		}
	}
	
	free( event );
	
	goto handle_events;
}

VkExtent2D control::extent() noexcept {
	return current_extent;
}

NativeSurfaceCreateInfo vk::surface::create_info() {
	return {
		.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.connection = com_xcb_connection,
		.window = com_xcb_window,
	};
}
