#include "zoo.h"
#include "zoo_sidebar.h"
#include "zoo_sound_pcm.h"
#include "zoo_ui.h"
#include "zoo_ui_input.h"
#include "types.h"
#include "render_software.h"
#include "sharp_bilinear.h"
#include "win32_io.h"

#include <windows.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef ERROR
#undef ERROR
#endif

#define WINDOW_CLASS_NAME L"ZZTNativeWindow"
#define NATIVE_WIDTH (80 * 8)
#define NATIVE_HEIGHT (25 * 14)
#define AUDIO_RATE 48000
#define AUDIO_BUFFER_SAMPLES 480
#define AUDIO_BUFFER_COUNT 4

static const uint32_t g_palette[16] = {
	0xff000000, 0xff0000aa, 0xff00aa00, 0xff00aaaa,
	0xffaa0000, 0xffaa00aa, 0xffaa5500, 0xffaaaaaa,
	0xff555555, 0xff5555ff, 0xff55ff55, 0xff55ffff,
	0xffff5555, 0xffff55ff, 0xffffff55, 0xffffffff
};

extern uint8_t res_8x14_bin[];

static zoo_state g_zoo;
static zoo_ui_state g_ui;
static zoo_io_path_driver g_io;
static zoo_sound_pcm_driver g_pcm;
static zoo_video_driver g_video_driver;
static uint8_t g_cells[80 * 25 * 2];
static uint32_t g_native_pixels[NATIVE_WIDTH * NATIVE_HEIGHT];
static uint32_t *g_scaled_pixels;
static size_t g_scaled_pixel_capacity;
static video_buffer g_video = { 80, 25, g_cells };
static render_options g_render_options;
static BITMAPINFO g_native_bitmap_info;
static BITMAPINFO g_scaled_bitmap_info;
static CRITICAL_SECTION g_state_lock;
static CRITICAL_SECTION g_audio_lock;
static HANDLE g_stop_event;
static HANDLE g_audio_stop_event;
static HANDLE g_game_thread;
static HANDLE g_pit_thread;
static HANDLE g_audio_thread;
static HWAVEOUT g_wave_out;
static WAVEHDR g_audio_headers[AUDIO_BUFFER_COUNT];
static uint8_t g_audio_buffers[AUDIO_BUFFER_COUNT][AUDIO_BUFFER_SAMPLES];
static HWND g_window;
static bool g_fullscreen;
static bool g_keyboard_down[256];
static uint16_t g_pressed_keys[256];
static LONG g_shutdown_started;
static WINDOWPLACEMENT g_saved_placement = { sizeof(WINDOWPLACEMENT) };
static DWORD g_saved_style;
static DWORD g_saved_ex_style;

typedef enum {
	CONFIRM_NONE,
	CONFIRM_QUIT_TITLE,
	CONFIRM_QUIT_PLAY
} confirmation_mode;

static confirmation_mode g_confirmation_mode;
static bool g_paused_before_confirmation;
static bool g_speed_prompt_active;

static void process_frontend_commands(void);
static void open_runtime_text_file(const char *filename, const char *title);
static bool route_title_key(WPARAM virtual_key, bool pressed);
static bool route_gameplay_key(WPARAM virtual_key, bool pressed);

static void video_write(zoo_video_driver *driver, int16_t x, int16_t y, uint8_t color, uint8_t character) {
	size_t pos;
	(void) driver;
	if (x < 0 || x >= 80 || y < 0 || y >= 25) return;
	pos = ((size_t) y * 80 + (size_t) x) * 2;
	g_cells[pos] = character;
	g_cells[pos + 1] = color;
}

static void video_read(zoo_video_driver *driver, int16_t x, int16_t y, uint8_t *color, uint8_t *character) {
	size_t pos;
	(void) driver;
	if (x < 0 || x >= 80 || y < 0 || y >= 25) {
		*color = 0;
		*character = ' ';
		return;
	}
	pos = ((size_t) y * 80 + (size_t) x) * 2;
	*color = g_cells[pos + 1];
	*character = g_cells[pos];
}

static void render_native_framebuffer(void) {
	/* Keep the actual 80x25 8x14 CP437 raster (640x350) untouched here.
	   The 4:3 CRT pixel-aspect correction belongs in the final filtered scale;
	   pre-stretching this image to 640x480 with nearest-neighbor creates uneven
	   repeated scanlines before the sharp-bilinear pass can smooth them. */
	software_draw_screen(&g_render_options, g_native_pixels,
		NATIVE_WIDTH * (int) sizeof(uint32_t), &g_video);
}

static bool ensure_scaled_pixel_buffer(int width, int height) {
	size_t pixel_count;
	size_t required;
	uint32_t *replacement;
	if (width <= 0 || height <= 0 || (size_t) width > SIZE_MAX / (size_t) height) return false;
	pixel_count = (size_t) width * (size_t) height;
	if (pixel_count > SIZE_MAX / sizeof(uint32_t)) return false;
	required = pixel_count * sizeof(uint32_t);
	if (required > UINT32_MAX) return false;
	if (pixel_count <= g_scaled_pixel_capacity) return true;
	replacement = (uint32_t *) realloc(g_scaled_pixels, required);
	if (replacement == NULL) return false;
	g_scaled_pixels = replacement;
	g_scaled_pixel_capacity = pixel_count;
	return true;
}

static bool render_scaled_frame(int width, int height) {
	if (!ensure_scaled_pixel_buffer(width, height)
		|| !zzt_sharp_bilinear_scale(g_native_pixels, NATIVE_WIDTH, NATIVE_HEIGHT,
			g_scaled_pixels, width, height, NULL, NULL)) return false;

	memset(&g_scaled_bitmap_info, 0, sizeof(g_scaled_bitmap_info));
	g_scaled_bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	g_scaled_bitmap_info.bmiHeader.biWidth = width;
	g_scaled_bitmap_info.bmiHeader.biHeight = -height;
	g_scaled_bitmap_info.bmiHeader.biPlanes = 1;
	g_scaled_bitmap_info.bmiHeader.biBitCount = 32;
	g_scaled_bitmap_info.bmiHeader.biCompression = BI_RGB;
	g_scaled_bitmap_info.bmiHeader.biSizeImage = (DWORD) ((size_t) width * (size_t) height * sizeof(uint32_t));
	return true;
}

static void draw_scaled_frame(HDC dc, const RECT *target, int width, int height) {
	if (render_scaled_frame(width, height)) {
		int lines = SetDIBitsToDevice(dc,
			target->left, target->top, (DWORD) width, (DWORD) height,
			0, 0, 0, (UINT) height, g_scaled_pixels,
			&g_scaled_bitmap_info, DIB_RGB_COLORS);
		if (lines > 0) return;
	}

	/* Allocation failure keeps the window usable with a crisp nearest fallback. */
	SetStretchBltMode(dc, COLORONCOLOR);
	StretchDIBits(dc,
		target->left, target->top, width, height,
		0, 0, NATIVE_WIDTH, NATIVE_HEIGHT,
		g_native_pixels, &g_native_bitmap_info, DIB_RGB_COLORS, SRCCOPY);
}

static void destination_rect(const RECT *client, RECT *dest) {
	int width = client->right - client->left;
	int height = client->bottom - client->top;
	int x;
	int y;
	int dest_width;
	int dest_height;
	if (!zzt_fit_4_3_viewport(width, height, &x, &y, &dest_width, &dest_height)) {
		SetRectEmpty(dest);
		return;
	}
	dest->left = client->left + x;
	dest->top = client->top + y;
	dest->right = dest->left + dest_width;
	dest->bottom = dest->top + dest_height;
}

static void paint_window(HWND window) {
	PAINTSTRUCT paint;
	HDC dc = BeginPaint(window, &paint);
	HDC frame_dc;
	HBITMAP frame_bitmap;
	HGDIOBJ previous_bitmap;
	RECT client;
	RECT target;
	HBRUSH black = (HBRUSH) GetStockObject(BLACK_BRUSH);
	int width;
	int height;
	int target_width;
	int target_height;
	GetClientRect(window, &client);
	width = client.right - client.left;
	height = client.bottom - client.top;
	if (width <= 0 || height <= 0) {
		EndPaint(window, &paint);
		return;
	}
	destination_rect(&client, &target);
	target_width = target.right - target.left;
	target_height = target.bottom - target.top;

	/* Compose the clear, letterboxing, and scaled screen off-screen. A single
	   BitBlt presents the completed frame, avoiding visible black between the
	   background clear and the DOS raster during blink-phase redraws. */
	frame_dc = CreateCompatibleDC(dc);
	frame_bitmap = frame_dc != NULL ? CreateCompatibleBitmap(dc, width, height) : NULL;
	if (frame_dc != NULL && frame_bitmap != NULL) {
		previous_bitmap = SelectObject(frame_dc, frame_bitmap);
		FillRect(frame_dc, &client, black);
		EnterCriticalSection(&g_state_lock);
		render_native_framebuffer();
		LeaveCriticalSection(&g_state_lock);
		draw_scaled_frame(frame_dc, &target, target_width, target_height);
		BitBlt(dc, 0, 0, width, height, frame_dc, 0, 0, SRCCOPY);
		SelectObject(frame_dc, previous_bitmap);
		DeleteObject(frame_bitmap);
		DeleteDC(frame_dc);
	} else {
		if (frame_bitmap != NULL) DeleteObject(frame_bitmap);
		if (frame_dc != NULL) DeleteDC(frame_dc);
		FillRect(dc, &client, black);
		EnterCriticalSection(&g_state_lock);
		render_native_framebuffer();
		LeaveCriticalSection(&g_state_lock);
		draw_scaled_frame(dc, &target, target_width, target_height);
	}
	EndPaint(window, &paint);
}

static void update_shoot_state(void) {
	bool shift = g_keyboard_down[VK_SHIFT] || g_keyboard_down[VK_LSHIFT] || g_keyboard_down[VK_RSHIFT];
	EnterCriticalSection(&g_state_lock);
	zoo_input_action_set(&g_zoo.input, ZOO_ACTION_SHOOT, shift);
	LeaveCriticalSection(&g_state_lock);
}

static uint16_t uppercase_ascii(uint16_t key) {
	return (key >= 'a' && key <= 'z') ? (uint16_t) (key - ('a' - 'A')) : key;
}

static uint16_t shifted_ascii(uint16_t value) {
	if (value >= 'a' && value <= 'z') return value - ('a' - 'A');
	switch (value) {
		case '1': return '!'; case '2': return '@'; case '3': return '#';
		case '4': return '$'; case '5': return '%'; case '6': return '^';
		case '7': return '&'; case '8': return '*'; case '9': return '(';
		case '0': return ')'; case '-': return '_'; case '=': return '+';
		case '[': return '{'; case ']': return '}'; case ';': return ':';
		case '\'': return '"'; case '\\': return '|'; case ',': return '<';
		case '.': return '>'; case '/': return '?'; case '`': return '~';
		default: return value;
	}
}

static uint16_t map_virtual_key(WPARAM key) {
	bool shift = g_keyboard_down[VK_SHIFT] || g_keyboard_down[VK_LSHIFT] || g_keyboard_down[VK_RSHIFT];
	bool caps = (GetKeyState(VK_CAPITAL) & 1) != 0;
	uint16_t ascii = 0;
	if (key >= 'A' && key <= 'Z') {
		ascii = (uint16_t) (caps ^ shift ? key : key + ('a' - 'A'));
	} else if (key >= '0' && key <= '9') {
		ascii = (uint16_t) key;
	} else {
		switch (key) {
			case VK_SPACE: ascii = ' '; break;
			case VK_OEM_1: ascii = ';'; break;
			case VK_OEM_PLUS: ascii = '='; break;
			case VK_OEM_COMMA: ascii = ','; break;
			case VK_OEM_MINUS: ascii = '-'; break;
			case VK_OEM_PERIOD: ascii = '.'; break;
			case VK_OEM_2: ascii = '/'; break;
			case VK_OEM_3: ascii = '`'; break;
			case VK_OEM_4: ascii = '['; break;
			case VK_OEM_5: ascii = '\\'; break;
			case VK_OEM_6: ascii = ']'; break;
			case VK_OEM_7: ascii = '\''; break;
			case VK_OEM_102: ascii = '\\'; break;
			case VK_ADD: ascii = '+'; break;
			case VK_SUBTRACT: ascii = '-'; break;
			case VK_MULTIPLY: ascii = '*'; break;
			case VK_DIVIDE: ascii = '/'; break;
			case VK_DECIMAL: ascii = '.'; break;
		}
	}
	if (ascii != 0 && !(key >= 'A' && key <= 'Z' && caps)) {
		if (shift) ascii = shifted_ascii(ascii);
	}
	if (key >= VK_F1 && key <= VK_F10) return (uint16_t) (ZOO_KEY_F1 + key - VK_F1);
	switch (key) {
		case VK_UP: return ZOO_KEY_UP;
		case VK_LEFT: return ZOO_KEY_LEFT;
		case VK_RIGHT: return ZOO_KEY_RIGHT;
		case VK_DOWN: return ZOO_KEY_DOWN;
		case VK_PRIOR: return ZOO_KEY_PAGE_UP;
		case VK_NEXT: return ZOO_KEY_PAGE_DOWN;
		case VK_INSERT: return ZOO_KEY_INSERT;
		case VK_DELETE: return ZOO_KEY_DELETE;
		case VK_HOME: return ZOO_KEY_HOME;
		case VK_END: return ZOO_KEY_END;
		case VK_BACK: return ZOO_KEY_BACKSPACE;
		case VK_TAB: return ZOO_KEY_TAB;
		case VK_RETURN: return ZOO_KEY_ENTER;
		case VK_ESCAPE: return ZOO_KEY_ESCAPE;
	}

	/* With NumLock off, the PC keypad's navigation keys act as arrows. */
	if ((GetKeyState(VK_NUMLOCK) & 1) == 0) {
		switch (key) {
			case VK_NUMPAD8: return ZOO_KEY_UP;
			case VK_NUMPAD2: return ZOO_KEY_DOWN;
			case VK_NUMPAD4: return ZOO_KEY_LEFT;
			case VK_NUMPAD6: return ZOO_KEY_RIGHT;
			case VK_NUMPAD9: return ZOO_KEY_PAGE_UP;
			case VK_NUMPAD3: return ZOO_KEY_PAGE_DOWN;
			case VK_NUMPAD7: return ZOO_KEY_HOME;
			case VK_NUMPAD1: return ZOO_KEY_END;
			case VK_NUMPAD0: return ZOO_KEY_INSERT;
			case VK_DECIMAL: return ZOO_KEY_DELETE;
		}
	} else if (key >= VK_NUMPAD0 && key <= VK_NUMPAD9) {
		return (uint16_t) ('0' + key - VK_NUMPAD0);
	}
	return ascii;
}

static void clear_sidebar_prompt_line(int y) {
	int x;
	for (x = 60; x < 80; x++) video_write(&g_video_driver, (int16_t) x, (int16_t) y, 0x11, ' ');
}

static void draw_speed_prompt(bool editable) {
	int x;
	clear_sidebar_prompt_line(21);
	clear_sidebar_prompt_line(22);
	clear_sidebar_prompt_line(23);
	for (x = 0; "Game speed:"[x] != '\0'; x++) {
		video_write(&g_video_driver, (int16_t) (66 + x), 21,
			(uint8_t) (editable ? 0x1F : 0x1E), (uint8_t) "Game speed:"[x]);
	}
	for (x = 0; "F....:....S"[x] != '\0'; x++) {
		video_write(&g_video_driver, (int16_t) (66 + x), 23, 0x1E,
			(uint8_t) "F....:....S"[x]);
	}
	video_write(&g_video_driver, (int16_t) (67 + g_zoo.tick_speed), 22,
		(uint8_t) (editable ? 0x9F : 0x1F), 31);
}

static void draw_confirmation_prompt(const char *message, confirmation_mode mode) {
	size_t i;
	clear_sidebar_prompt_line(3);
	clear_sidebar_prompt_line(4);
	clear_sidebar_prompt_line(5);
	for (i = 0; message[i] != '\0' && 63 + i < 80; i++) {
		video_write(&g_video_driver, (int16_t) (63 + i), 5, 0x1F, (uint8_t) message[i]);
	}
	if (63 + i < 80) video_write(&g_video_driver, (int16_t) (63 + i), 5, 0x9E, '_');
	g_confirmation_mode = mode;
}

static void resolve_confirmation(uint16_t key) {
	confirmation_mode mode = g_confirmation_mode;
	bool accepted = (key == 'Y' || key == 'y');
	g_confirmation_mode = CONFIRM_NONE;
	clear_sidebar_prompt_line(5);
	if (mode == CONFIRM_QUIT_TITLE) {
		zoo_game_resolve_title_quit(&g_zoo, accepted);
		if (accepted) PostMessageW(g_window, WM_CLOSE, 0, 0);
	} else if (mode == CONFIRM_QUIT_PLAY) {
		zoo_game_resolve_end_play(&g_zoo, accepted, g_paused_before_confirmation);
		if (accepted) zoo_world_return_title(&g_zoo);
	}
	InvalidateRect(g_window, NULL, FALSE);
}

static bool route_confirmation_key(WPARAM virtual_key, bool pressed) {
	uint16_t key = 0;
	bool handled = false;
	if (virtual_key >= 256) return false;
	EnterCriticalSection(&g_state_lock);
	if (g_confirmation_mode != CONFIRM_NONE) {
		handled = true;
		if (pressed) {
			key = map_virtual_key(virtual_key);
			g_keyboard_down[virtual_key] = true;
			g_pressed_keys[virtual_key] = 0;
			if (key >= 'a' && key <= 'z') key = (uint16_t) (key - ('a' - 'A'));
			if (key == 'Y' || key == 'N' || key == ZOO_KEY_ESCAPE) resolve_confirmation(key);
		} else {
			g_keyboard_down[virtual_key] = false;
			g_pressed_keys[virtual_key] = 0;
		}
	}
	LeaveCriticalSection(&g_state_lock);
	return handled;
}

static void send_key(WPARAM virtual_key, bool pressed) {
	uint16_t key;
	if (virtual_key >= 256) return;
	if (route_confirmation_key(virtual_key, pressed)) return;
	if (route_title_key(virtual_key, pressed)) return;
	if (route_gameplay_key(virtual_key, pressed)) return;
	if (pressed) {
		if (virtual_key == VK_SHIFT || virtual_key == VK_LSHIFT || virtual_key == VK_RSHIFT) {
			g_keyboard_down[virtual_key] = true;
			key = 0;
		} else {
			key = map_virtual_key(virtual_key);
			g_keyboard_down[virtual_key] = true;
			g_pressed_keys[virtual_key] = key;
		}
	} else {
		key = g_pressed_keys[virtual_key];
		g_keyboard_down[virtual_key] = false;
		g_pressed_keys[virtual_key] = 0;
	}
	if (key != 0) {
		EnterCriticalSection(&g_state_lock);
		zoo_ui_input_key(&g_zoo, &g_ui.input, key, pressed);
		LeaveCriticalSection(&g_state_lock);
	}
	update_shoot_state();
}

static void release_all_keys(void) {
	int key_id;
	EnterCriticalSection(&g_state_lock);
	for (key_id = 0; key_id < 256; key_id++) {
		if (g_pressed_keys[key_id] != 0) {
			zoo_ui_input_key(&g_zoo, &g_ui.input, g_pressed_keys[key_id], false);
		}
		g_pressed_keys[key_id] = 0;
		g_keyboard_down[key_id] = false;
	}
	for (key_id = 0; key_id < ZOO_ACTION_MAX; key_id++) {
		zoo_input_action_set(&g_zoo.input, (zoo_input_action) key_id, false);
	}
	LeaveCriticalSection(&g_state_lock);
}

static void toggle_fullscreen(HWND window) {
	if (!g_fullscreen) {
		MONITORINFO monitor_info;
		HMONITOR monitor;
		RECT monitor_rect;
		g_saved_placement.length = sizeof(g_saved_placement);
		GetWindowPlacement(window, &g_saved_placement);
		g_saved_style = (DWORD) GetWindowLongPtr(window, GWL_STYLE);
		g_saved_ex_style = (DWORD) GetWindowLongPtr(window, GWL_EXSTYLE);
		monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
		monitor_info.cbSize = sizeof(monitor_info);
		GetMonitorInfo(monitor, &monitor_info);
		monitor_rect = monitor_info.rcMonitor;
		SetWindowLongPtr(window, GWL_STYLE, g_saved_style & ~(WS_OVERLAPPEDWINDOW));
		SetWindowLongPtr(window, GWL_EXSTYLE, g_saved_ex_style & ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));
		SetWindowPos(window, HWND_TOP, monitor_rect.left, monitor_rect.top,
			monitor_rect.right - monitor_rect.left, monitor_rect.bottom - monitor_rect.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
		g_fullscreen = true;
	} else {
		SetWindowLongPtr(window, GWL_STYLE, g_saved_style);
		SetWindowLongPtr(window, GWL_EXSTYLE, g_saved_ex_style);
		SetWindowPlacement(window, &g_saved_placement);
		SetWindowPos(window, NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		g_fullscreen = false;
	}
	InvalidateRect(window, NULL, TRUE);
}

static void fix_sizing_rect(HWND window, RECT *rect, UINT edge) {
	RECT outer;
	RECT client;
	LONG frame_width;
	LONG frame_height;
	LONG outer_width = rect->right - rect->left;
	LONG outer_height = rect->bottom - rect->top;
	LONG client_width;
	LONG client_height;
	LONG new_outer_width;
	LONG new_outer_height;
	bool adjust_width = edge == WMSZ_TOP || edge == WMSZ_BOTTOM
		|| edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT
		|| edge == WMSZ_BOTTOMLEFT || edge == WMSZ_BOTTOMRIGHT;
	if (g_fullscreen || !GetWindowRect(window, &outer) || !GetClientRect(window, &client)) return;
	frame_width = (outer.right - outer.left) - (client.right - client.left);
	frame_height = (outer.bottom - outer.top) - (client.bottom - client.top);
	client_width = outer_width - frame_width;
	client_height = outer_height - frame_height;
	if (adjust_width) {
		client_width = client_height * 4 / 3;
	} else {
		client_height = client_width * 3 / 4;
	}
	new_outer_width = client_width + frame_width;
	new_outer_height = client_height + frame_height;
	switch (edge) {
		case WMSZ_LEFT: rect->left = rect->right - new_outer_width; rect->bottom = rect->top + new_outer_height; break;
		case WMSZ_RIGHT: rect->right = rect->left + new_outer_width; rect->bottom = rect->top + new_outer_height; break;
		case WMSZ_TOP: rect->top = rect->bottom - new_outer_height; rect->right = rect->left + new_outer_width; break;
		case WMSZ_BOTTOM: rect->bottom = rect->top + new_outer_height; rect->right = rect->left + new_outer_width; break;
		case WMSZ_TOPLEFT: rect->left = rect->right - new_outer_width; rect->top = rect->bottom - new_outer_height; break;
		case WMSZ_TOPRIGHT: rect->right = rect->left + new_outer_width; rect->top = rect->bottom - new_outer_height; break;
		case WMSZ_BOTTOMLEFT: rect->left = rect->right - new_outer_width; rect->bottom = rect->top + new_outer_height; break;
		case WMSZ_BOTTOMRIGHT: rect->right = rect->left + new_outer_width; rect->bottom = rect->top + new_outer_height; break;
	}
}

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
	switch (message) {
		case WM_PAINT:
			paint_window(window);
			return 0;
		case WM_ERASEBKGND:
			return 1;
		case WM_TIMER:
			if (wparam == 1) {
				static DWORD last_blink_phase = 0xffffffffUL;
				DWORD phase = (GetTickCount() / 500) & 1;
				g_render_options.flags = phase ? RENDER_BLINK_PHASE : 0;
				if (phase != last_blink_phase) {
					last_blink_phase = phase;
					/* Redraw only when DOS blink attributes change phase. Repainting
					   every 16 ms exposed the black background clear as a screen flash. */
					InvalidateRect(window, NULL, FALSE);
				}
			}
			return 0;
		case WM_SIZING:
			fix_sizing_rect(window, (RECT *) lparam, (UINT) wparam);
			return TRUE;
		case WM_GETMINMAXINFO: {
			MINMAXINFO *info = (MINMAXINFO *) lparam;
			RECT min_rect = { 0, 0, 640, 480 };
			AdjustWindowRectEx(&min_rect, (DWORD) GetWindowLongPtr(window, GWL_STYLE), FALSE,
				(DWORD) GetWindowLongPtr(window, GWL_EXSTYLE));
			info->ptMinTrackSize.x = min_rect.right - min_rect.left;
			info->ptMinTrackSize.y = min_rect.bottom - min_rect.top;
			return 0;
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if ((message == WM_SYSKEYDOWN && wparam == VK_RETURN && (GetKeyState(VK_MENU) & 0x8000))
				|| (message == WM_KEYDOWN && wparam == VK_RETURN && (GetKeyState(VK_MENU) & 0x8000))) {
				toggle_fullscreen(window);
				return 0;
			}
			send_key(wparam, true);
			return message == WM_SYSKEYDOWN ? DefWindowProc(window, message, wparam, lparam) : 0;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			send_key(wparam, false);
			return message == WM_SYSKEYUP ? DefWindowProc(window, message, wparam, lparam) : 0;
		case WM_ACTIVATEAPP:
			if (!wparam) release_all_keys();
			return 0;
		case WM_CLOSE:
			if (InterlockedCompareExchange(&g_shutdown_started, 1, 0) == 0) {
				SetEvent(g_stop_event);
				SetEvent(g_audio_stop_event);
			}
			DestroyWindow(window);
			return 0;
		case WM_DESTROY:
			KillTimer(window, 1);
			PostQuitMessage(0);
			return 0;
	}
	return DefWindowProc(window, message, wparam, lparam);
}

static DWORD WINAPI game_thread_main(void *unused) {
	(void) unused;
	while (WaitForSingleObject(g_stop_event, 0) != WAIT_OBJECT_0) {
		DWORD delay = (DWORD) ZOO_PIT_TICK_MS;
		bool running = true;
		EnterCriticalSection(&g_state_lock);
		if (g_confirmation_mode == CONFIRM_NONE) {
			zoo_ui_tick(&g_ui);
			while (running) {
				switch (zoo_tick(&g_zoo)) {
					case RETURN_IMMEDIATE:
						break;
					case RETURN_NEXT_FRAME:
						delay = 16;
						running = false;
						break;
					case RETURN_NEXT_CYCLE:
						running = false;
						break;
					case EXIT:
					case ERROR:
						running = false;
						break;
				}
			}
			process_frontend_commands();
		} else {
			delay = 16;
		}
		LeaveCriticalSection(&g_state_lock);
		WaitForSingleObject(g_stop_event, delay ? delay : 1);
	}
	return 0;
}

static DWORD WINAPI pit_thread_main(void *unused) {
	LARGE_INTEGER frequency;
	LARGE_INTEGER now;
	double period;
	double next_tick;
	(void) unused;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&now);
	period = (double) frequency.QuadPart * ZOO_PIT_TICK_MS / 1000.0;
	next_tick = (double) now.QuadPart + period;
	while (WaitForSingleObject(g_stop_event, 0) != WAIT_OBJECT_0) {
		DWORD wait_ms = 1;
		QueryPerformanceCounter(&now);
		if ((double) now.QuadPart >= next_tick) {
			EnterCriticalSection(&g_state_lock);
			EnterCriticalSection(&g_audio_lock);
			zoo_tick_advance_pit(&g_zoo);
			zoo_sound_tick(&g_zoo.sound);
			zoo_sound_pcm_tick(&g_pcm);
			LeaveCriticalSection(&g_audio_lock);
			zoo_input_tick(&g_zoo.input);
			LeaveCriticalSection(&g_state_lock);
			next_tick += period;
		} else {
			double remain_ms = ((next_tick - (double) now.QuadPart) * 1000.0) / (double) frequency.QuadPart;
			if (remain_ms >= 2.0) wait_ms = (DWORD) (remain_ms - 1.0);
			WaitForSingleObject(g_stop_event, wait_ms);
		}
	}
	return 0;
}

static void fill_audio_buffer(int index) {
	EnterCriticalSection(&g_audio_lock);
	zoo_sound_pcm_generate(&g_pcm, g_audio_buffers[index], AUDIO_BUFFER_SAMPLES);
	LeaveCriticalSection(&g_audio_lock);
}

static DWORD WINAPI audio_thread_main(void *unused) {
	(void) unused;
	while (WaitForSingleObject(g_audio_stop_event, 0) != WAIT_OBJECT_0) {
		int i;
		for (i = 0; i < AUDIO_BUFFER_COUNT; i++) {
			if ((g_audio_headers[i].dwFlags & WHDR_DONE) != 0) {
				fill_audio_buffer(i);
				waveOutWrite(g_wave_out, &g_audio_headers[i], sizeof(WAVEHDR));
			}
		}
		WaitForSingleObject(g_audio_stop_event, 2);
	}
	return 0;
}

static void audio_start(void) {
	WAVEFORMATEX format;
	MMRESULT result;
	int i;
	memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 1;
	format.nSamplesPerSec = AUDIO_RATE;
	format.wBitsPerSample = 8;
	format.nBlockAlign = 1;
	format.nAvgBytesPerSec = AUDIO_RATE;
	format.cbSize = 0;
	g_pcm.frequency = AUDIO_RATE;
	g_pcm.channels = 1;
	g_pcm.volume = 200;
	g_pcm.latency = 1;
	g_pcm.format_signed = false;
	zoo_sound_pcm_init(&g_pcm);
	g_zoo.sound.d_sound = &g_pcm.parent;
	result = waveOutOpen(&g_wave_out, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL);
	if (result != MMSYSERR_NOERROR) {
		g_wave_out = NULL;
		return;
	}
	for (i = 0; i < AUDIO_BUFFER_COUNT; i++) {
		memset(g_audio_buffers[i], 128, sizeof(g_audio_buffers[i]));
		memset(&g_audio_headers[i], 0, sizeof(g_audio_headers[i]));
		g_audio_headers[i].lpData = (LPSTR) g_audio_buffers[i];
		g_audio_headers[i].dwBufferLength = AUDIO_BUFFER_SAMPLES;
		if (waveOutPrepareHeader(g_wave_out, &g_audio_headers[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			waveOutReset(g_wave_out);
			while (i > 0) waveOutUnprepareHeader(g_wave_out, &g_audio_headers[--i], sizeof(WAVEHDR));
			waveOutClose(g_wave_out);
			g_wave_out = NULL;
			return;
		}
	}
	for (i = 0; i < AUDIO_BUFFER_COUNT; i++) waveOutWrite(g_wave_out, &g_audio_headers[i], sizeof(WAVEHDR));
	g_audio_thread = CreateThread(NULL, 0, audio_thread_main, NULL, 0, NULL);
}

static void audio_stop(void) {
	int i;
	if (g_audio_thread != NULL) {
		SetEvent(g_audio_stop_event);
		WaitForSingleObject(g_audio_thread, INFINITE);
		CloseHandle(g_audio_thread);
		g_audio_thread = NULL;
	}
	if (g_wave_out != NULL) {
		waveOutReset(g_wave_out);
		for (i = 0; i < AUDIO_BUFFER_COUNT; i++) waveOutUnprepareHeader(g_wave_out, &g_audio_headers[i], sizeof(WAVEHDR));
		waveOutClose(g_wave_out);
		g_wave_out = NULL;
	}
}

static bool load_world_path(const char *path, bool show_error) {
	zoo_io_handle handle;
	int result;
	const char *extension = strrchr(path, '.');
	bool as_save = extension != NULL && _stricmp(extension, ".SAV") == 0;
	if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) return false;
	handle = g_io.func_open_file_absolute(&g_io, path, MODE_READ);
	if (handle.p == NULL) {
		if (handle.func_close != NULL) handle.func_close(&handle);
		return false;
	}
	EnterCriticalSection(&g_state_lock);
	result = zoo_world_load(&g_zoo, &handle, false);
	if (result == 0) {
		if (as_save) {
			zoo_game_start(&g_zoo, GS_PLAY);
		} else {
			zoo_board_change(&g_zoo, 0);
			zoo_game_start(&g_zoo, GS_TITLE);
		}
		zoo_redraw(&g_zoo);
	}
	LeaveCriticalSection(&g_state_lock);
	handle.func_close(&handle);
	if (result != 0) {
		if (show_error) MessageBoxW(g_window, L"ZZT could not load this world file.", L"ZZT", MB_OK | MB_ICONWARNING);
		return false;
	}
	return true;
}

static bool try_load_argument_world(const wchar_t *argument) {
	char path[ZOO_PATH_MAX + 1];
	int bytes;
	if (argument == NULL || argument[0] == L'\0') return false;
	bytes = WideCharToMultiByte(CP_ACP, 0, argument, -1, path, sizeof(path), NULL, NULL);
	if (bytes <= 0) {
		MessageBoxW(g_window, L"The world file path is too long or cannot be represented in the active Windows code page.",
			L"ZZT", MB_OK | MB_ICONWARNING);
		return false;
	}
	return load_world_path(path, true);
}

static void try_load_default_demo(void) {
	char path[ZOO_PATH_MAX + 1];
	DWORD length;
	char *separator;
	if (load_world_path("DEMO.ZZT", false)) return;
	length = GetModuleFileNameA(NULL, path, sizeof(path));
	if (length == 0 || length >= sizeof(path)) return;
	separator = strrchr(path, '\\');
	if (separator == NULL) separator = strrchr(path, '/');
	if (separator == NULL || (size_t) (separator - path) + 11 >= sizeof(path)) return;
	strcpy(separator + 1, "DEMO.ZZT");
	load_world_path(path, false);
}

static void open_runtime_text_file(const char *filename, const char *title) {
	char path[ZOO_PATH_MAX + 1];
	DWORD length;
	char *separator;
	zoo_io_handle handle;

	zoo_window_close(&g_ui.window);
	memset(&g_ui.window, 0, sizeof(g_ui.window));
	strncpy(g_ui.window.title, title, sizeof(g_ui.window.title) - 1);
	length = GetModuleFileNameA(NULL, path, sizeof(path));
	if (length > 0 && length < sizeof(path)) {
		separator = strrchr(path, '\\');
		if (separator == NULL) separator = strrchr(path, '/');
		if (separator != NULL && (size_t) (separator - path) + strlen(filename) + 2 < sizeof(path)) {
			strcpy(separator + 1, filename);
			handle = g_io.func_open_file_absolute(&g_io, path, MODE_READ);
			if (handle.p != NULL) {
				zoo_window_append_file(&g_ui.window, &handle);
				handle.func_close(&handle);
			} else {
				if (handle.func_close != NULL) handle.func_close(&handle);
				zoo_window_append(&g_ui.window, "Help file could not be opened.");
			}
		} else {
			zoo_window_append(&g_ui.window, "Help file path is unavailable.");
		}
	} else {
		zoo_window_append(&g_ui.window, "Help file path is unavailable.");
	}
	g_ui.window.viewing_file = true;
	zoo_window_open(&g_zoo, &g_ui.window);
}

static bool route_title_key(WPARAM virtual_key, bool pressed) {
	uint16_t key;
	bool handled = false;
	bool shift_held;
	if (virtual_key >= 256) return false;

	EnterCriticalSection(&g_state_lock);
	if (g_speed_prompt_active) {
		handled = true;
		if (pressed) {
			g_keyboard_down[virtual_key] = true;
			g_pressed_keys[virtual_key] = 0;
			key = uppercase_ascii(map_virtual_key(virtual_key));
			shift_held = g_keyboard_down[VK_SHIFT] || g_keyboard_down[VK_LSHIFT]
				|| g_keyboard_down[VK_RSHIFT];
			if (key >= '1' && key <= '9') {
				g_zoo.tick_speed = (int16_t) (key - '1');
			} else if (key == ZOO_KEY_LEFT && g_zoo.tick_speed > 0) {
				g_zoo.tick_speed--;
			} else if (key == ZOO_KEY_RIGHT && g_zoo.tick_speed < 8) {
				g_zoo.tick_speed++;
			}
			if (key == ZOO_KEY_ENTER || key == ZOO_KEY_ESCAPE
				|| (shift_held && (key == ZOO_KEY_LEFT || key == ZOO_KEY_RIGHT))) {
				g_speed_prompt_active = false;
			}
			draw_speed_prompt(g_speed_prompt_active);
		} else {
			g_keyboard_down[virtual_key] = false;
			g_pressed_keys[virtual_key] = 0;
		}
		LeaveCriticalSection(&g_state_lock);
		InvalidateRect(g_window, NULL, FALSE);
		return handled;
	}
	if (!pressed) {
		LeaveCriticalSection(&g_state_lock);
		return false;
	}
	key = uppercase_ascii(map_virtual_key(virtual_key));
	if (key != 'P' && key != 'W' && key != 'R' && key != 'A' && key != 'Q'
		&& key != 'S' && key != ZOO_KEY_ESCAPE) {
		LeaveCriticalSection(&g_state_lock);
		return false;
	}
	if (g_zoo.game_state == GS_TITLE && zoo_call_empty(&g_zoo.call_stack)) {
		g_keyboard_down[virtual_key] = true;
		g_pressed_keys[virtual_key] = 0;
		handled = true;
		switch (key) {
			case 'P': zoo_world_play(&g_zoo); break;
			case 'W': zoo_ui_load_world(&g_ui, false); break;
			case 'R': zoo_ui_load_world(&g_ui, true); break;
			case 'A': open_runtime_text_file("ABOUT.HLP", "About ZZT..."); break;
			case 'S':
				g_speed_prompt_active = true;
				draw_speed_prompt(true);
				break;
			case 'Q':
			case ZOO_KEY_ESCAPE:
				draw_confirmation_prompt("Quit ZZT? ", CONFIRM_QUIT_TITLE);
				break;
		}
	}
	LeaveCriticalSection(&g_state_lock);
	if (handled) InvalidateRect(g_window, NULL, FALSE);
	return handled;
}

static bool route_gameplay_key(WPARAM virtual_key, bool pressed) {
	uint16_t key;
	bool handled = false;
	if (!pressed || virtual_key >= 256) return false;
	key = uppercase_ascii(map_virtual_key(virtual_key));
	if (key != 'Q' && key != ZOO_KEY_ESCAPE && key != 'S' && key != 'H' && key != 'F') return false;

	EnterCriticalSection(&g_state_lock);
	if (g_zoo.game_state == GS_PLAY && zoo_call_empty(&g_zoo.call_stack)
		&& g_confirmation_mode == CONFIRM_NONE) {
		g_keyboard_down[virtual_key] = true;
		g_pressed_keys[virtual_key] = 0;
		handled = true;
		switch (key) {
			case 'Q':
			case ZOO_KEY_ESCAPE:
				if (g_zoo.world.info.health <= 0) {
					zoo_world_return_title(&g_zoo);
				} else {
					g_paused_before_confirmation = g_zoo.game_paused;
					g_zoo.game_paused = true;
					draw_confirmation_prompt("End this game? ", CONFIRM_QUIT_PLAY);
				}
				break;
			case 'S': zoo_ui_save_world(&g_ui); break;
			case 'H': open_runtime_text_file("GAME.HLP", "Playing ZZT"); break;
			case 'F': open_runtime_text_file("ORDER.HLP", "Order form"); break;
		}
	}
	LeaveCriticalSection(&g_state_lock);
	return handled;
}

static void process_frontend_commands(void) {
	uint16_t key = uppercase_ascii(g_zoo.input.key_pressed);
	if (!zoo_call_empty(&g_zoo.call_stack) || g_confirmation_mode != CONFIRM_NONE) return;

	if (g_zoo.game_state == GS_TITLE && g_zoo.game_play_exit_requested) {
		g_zoo.game_play_exit_requested = false;
		g_zoo.input.key_pressed = 0;
		switch (key) {
			case 'P': zoo_world_play(&g_zoo); break;
			case 'W': zoo_ui_load_world(&g_ui, false); break;
			case 'R': zoo_ui_load_world(&g_ui, true); break;
			case 'A': open_runtime_text_file("ABOUT.HLP", "About ZZT..."); break;
			case 'Q':
			case ZOO_KEY_ESCAPE:
				draw_confirmation_prompt("Quit ZZT? ", CONFIRM_QUIT_TITLE);
				break;
			default: break;
		}
		return;
	}

	if (g_zoo.game_state != GS_PLAY) return;
	if (g_zoo.game_play_exit_requested && g_zoo.world.info.health <= 0) {
		g_zoo.game_play_exit_requested = false;
		zoo_world_return_title(&g_zoo);
		return;
	}
	if (key == 'Q' || key == ZOO_KEY_ESCAPE) {
		g_zoo.input.key_pressed = 0;
		if (g_zoo.world.info.health <= 0) {
			zoo_world_return_title(&g_zoo);
		} else {
			g_paused_before_confirmation = g_zoo.game_paused;
			g_zoo.game_paused = true;
			draw_confirmation_prompt("End this game? ", CONFIRM_QUIT_PLAY);
		}
	} else if (key == 'S') {
		g_zoo.input.key_pressed = 0;
		zoo_ui_save_world(&g_ui);
	} else if (key == 'H') {
		g_zoo.input.key_pressed = 0;
		open_runtime_text_file("GAME.HLP", "Playing ZZT");
	} else if (key == 'F') {
		g_zoo.input.key_pressed = 0;
		open_runtime_text_file("ORDER.HLP", "Order form");
	}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show_command) {
	WNDCLASSEXW window_class;
	RECT initial_rect = { 0, 0, 960, 720 };
	MSG message;
	int argc = 0;
	LPWSTR *argv;
	const wchar_t *world_argument = NULL;
	int exit_code = 0;
	(void) previous;
	(void) command_line;

	SetProcessDPIAware();
	InitializeCriticalSection(&g_state_lock);
	InitializeCriticalSection(&g_audio_lock);
	memset(g_cells, 0, sizeof(g_cells));
	for (int i = 0; i < 80 * 25; i++) {
		g_cells[i * 2] = ' ';
		g_cells[i * 2 + 1] = 0x07;
	}

	g_video_driver.func_write = video_write;
	g_video_driver.func_read = video_read;
	zoo_io_create_win32_driver(&g_io);
	zoo_state_init(&g_zoo);
	g_zoo.d_video = &g_video_driver;
	g_zoo.d_io = &g_io.parent;
	g_zoo.func_draw_sidebar = zoo_draw_sidebar_classic;
	zoo_ui_init(&g_ui, &g_zoo);

	g_render_options.charset = res_8x14_bin;
	g_render_options.palette = g_palette;
	g_render_options.char_width = 8;
	g_render_options.char_height = 14;
	g_render_options.flags = 0;
	g_render_options.bpp = 32;
	memset(&g_native_bitmap_info, 0, sizeof(g_native_bitmap_info));
	g_native_bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	g_native_bitmap_info.bmiHeader.biWidth = NATIVE_WIDTH;
	g_native_bitmap_info.bmiHeader.biHeight = -NATIVE_HEIGHT;
	g_native_bitmap_info.bmiHeader.biPlanes = 1;
	g_native_bitmap_info.bmiHeader.biBitCount = 32;
	g_native_bitmap_info.bmiHeader.biCompression = BI_RGB;
	g_native_bitmap_info.bmiHeader.biSizeImage = sizeof(g_native_pixels);

	memset(&window_class, 0, sizeof(window_class));
	window_class.cbSize = sizeof(window_class);
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = window_proc;
	window_class.hInstance = instance;
	window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	window_class.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	window_class.lpszClassName = L"ZZTNativeWindow";
	if (!RegisterClassExW(&window_class)) {
		DeleteCriticalSection(&g_audio_lock);
		DeleteCriticalSection(&g_state_lock);
		return 1;
	}
	AdjustWindowRectEx(&initial_rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
	g_window = CreateWindowExW(0, WINDOW_CLASS_NAME, L"ZZT", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, initial_rect.right - initial_rect.left,
		initial_rect.bottom - initial_rect.top, NULL, NULL, instance, NULL);
	if (g_window == NULL) {
		DeleteCriticalSection(&g_audio_lock);
		DeleteCriticalSection(&g_state_lock);
		return 1;
	}
	SetWindowTextW(g_window, L"ZZT");

	g_stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
	g_audio_stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
	if (g_stop_event == NULL || g_audio_stop_event == NULL) {
		MessageBoxW(g_window, L"Unable to create the game timing events.", L"ZZT", MB_OK | MB_ICONERROR);
		DestroyWindow(g_window);
		exit_code = 1;
		goto cleanup;
	}

	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv != NULL && argc > 1) world_argument = argv[1];
	if (world_argument != NULL) try_load_argument_world(world_argument);
	else try_load_default_demo();
	if (argv != NULL) LocalFree(argv);

	EnterCriticalSection(&g_state_lock);
	zoo_redraw(&g_zoo);
	LeaveCriticalSection(&g_state_lock);
	ShowWindow(g_window, show_command);
	UpdateWindow(g_window);
	SetTimer(g_window, 1, 16, NULL);

	audio_start();
	g_game_thread = CreateThread(NULL, 0, game_thread_main, NULL, 0, NULL);
	g_pit_thread = CreateThread(NULL, 0, pit_thread_main, NULL, 0, NULL);
	if (g_game_thread == NULL || g_pit_thread == NULL) {
		MessageBoxW(g_window, L"Unable to start the game timing threads.", L"ZZT", MB_OK | MB_ICONERROR);
		SetEvent(g_stop_event);
		DestroyWindow(g_window);
		exit_code = 1;
		goto cleanup;
	}

	while (GetMessageW(&message, NULL, 0, 0) > 0) {
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}

cleanup:
	if (g_stop_event != NULL) SetEvent(g_stop_event);
	if (g_game_thread != NULL) {
		WaitForSingleObject(g_game_thread, INFINITE);
		CloseHandle(g_game_thread);
	}
	if (g_pit_thread != NULL) {
		WaitForSingleObject(g_pit_thread, INFINITE);
		CloseHandle(g_pit_thread);
	}
	audio_stop();
	if (g_audio_stop_event != NULL) CloseHandle(g_audio_stop_event);
	if (g_stop_event != NULL) CloseHandle(g_stop_event);
	free(g_scaled_pixels);
	g_scaled_pixels = NULL;
	g_scaled_pixel_capacity = 0;
	DeleteCriticalSection(&g_audio_lock);
	DeleteCriticalSection(&g_state_lock);
	return exit_code;
}
