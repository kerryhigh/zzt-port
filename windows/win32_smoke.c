#include <stdio.h>
#include <string.h>

#include "zoo.h"
#include "zoo_sidebar.h"
#include "win32_io.h"

#include <windows.h>

#ifdef ERROR
#undef ERROR
#endif

static zoo_state g_state;
static zoo_state g_restored;
static zoo_io_path_driver g_io;
static zoo_video_driver g_video;
static bool g_seen[25][80];
static uint8_t g_cells[25][80][2];
static int g_invalid_cell;

static void capture_cell(zoo_video_driver *driver, int16_t x, int16_t y, uint8_t color, uint8_t character) {
	(void) driver;
	if (x < 0 || x >= 80 || y < 0 || y >= 25) {
		g_invalid_cell++;
		return;
	}
	g_seen[y][x] = true;
	g_cells[y][x][0] = character;
	g_cells[y][x][1] = color;
}

static int expect_text(int x, int y, uint8_t color, const char *text) {
	int i;
	for (i = 0; text[i] != '\0'; i++) {
		if (x + i >= 80 || g_cells[y][x + i][0] != (uint8_t) text[i] || g_cells[y][x + i][1] != color) {
			fprintf(stderr, "screen mismatch at %d,%d: expected '%c' color %02x, got '%c' color %02x\n",
				x + i, y, text[i], color, g_cells[y][x + i][0], g_cells[y][x + i][1]);
			return 0;
		}
	}
	return 1;
}

static int save_state(zoo_state *state, const char *path) {
	zoo_io_handle handle = g_io.func_open_file_absolute(&g_io, path, MODE_WRITE);
	int result;
	if (handle.p == NULL) {
		if (handle.func_close != NULL) handle.func_close(&handle);
		return 1;
	}
	result = zoo_world_save(state, &handle);
	if (handle.func_close != NULL) handle.func_close(&handle);
	return result != 0;
}

static int files_equal(const char *first, const char *second) {
	FILE *a = fopen(first, "rb");
	FILE *b = fopen(second, "rb");
	int equal = 1;
	if (a == NULL || b == NULL) equal = 0;
	while (equal) {
		int ca = fgetc(a);
		int cb = fgetc(b);
		if (ca != cb) {
			equal = 0;
			break;
		}
		if (ca == EOF) break;
	}
	if (a != NULL) fclose(a);
	if (b != NULL) fclose(b);
	return equal;
}

int main(int argc, char **argv) {
	zoo_io_handle handle;
	DWORD attributes;
	int result;
	int x, y;
	if (argc != 4) {
		fprintf(stderr, "usage: zzt_windows_smoke <demo.zzt> <roundtrip-a.sav> <roundtrip-b.sav>\n");
		return 2;
	}

	zoo_io_create_win32_driver(&g_io);
	zoo_state_init(&g_state);
	g_state.d_io = &g_io.parent;
	g_video.func_write = capture_cell;
	g_state.d_video = &g_video;
	g_state.func_draw_sidebar = zoo_draw_sidebar_classic;
	handle = g_io.func_open_file_absolute(&g_io, argv[1], MODE_READ);
	if (handle.p == NULL) {
		fprintf(stderr, "failed to open demo world through Win32 driver\n");
		if (handle.func_close != NULL) handle.func_close(&handle);
		return 1;
	}
	result = zoo_world_load(&g_state, &handle, false);
	if (handle.func_close != NULL) handle.func_close(&handle);
	if (result != 0) {
		fprintf(stderr, "zoo_world_load failed: %d\n", result);
		return 1;
	}
	zoo_redraw(&g_state);
	if (g_invalid_cell != 0) {
		fprintf(stderr, "screen emitted %d out-of-range cells\n", g_invalid_cell);
		return 1;
	}
	for (y = 0; y < 25; y++) {
		for (x = 0; x < 80; x++) {
			if (!g_seen[y][x]) {
				fprintf(stderr, "80x25 redraw omitted cell %d,%d\n", x, y);
				return 1;
			}
		}
	}
	if (!expect_text(62, 1, 0x70, "      ZZT      ")
		|| !expect_text(62, 11, 0x70, " P ")
		|| !expect_text(65, 11, 0x1F, " Play")
		|| !expect_text(65, 12, 0x1E, " Restore game")
		|| !expect_text(65, 16, 0x1F, " About ZZT!")
		|| !expect_text(65, 17, 0x1E, " High Scores")
		|| !expect_text(66, 21, 0x1E, "Game speed:")
		|| !expect_text(66, 23, 0x1E, "F....:....S")) {
		return 1;
	}
	if (g_cells[22][71][0] != 31 || g_cells[22][71][1] != 0x1F) {
		fprintf(stderr, "default game-speed marker mismatch: expected 1f:31 at 71,22, got %02x:%02x\n",
			g_cells[22][71][1], g_cells[22][71][0]);
		return 1;
	}

	/* Match the Pascal state after the first accepted player move. */
	g_state.world.info.is_save = true;
	g_state.world.info.io_is_save_byte = 1;
	if (save_state(&g_state, argv[2])) {
		fprintf(stderr, "world save failed\n");
		return 1;
	}
	attributes = GetFileAttributesA(argv[2]);
	if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		fprintf(stderr, "save output was not created\n");
		return 1;
	}
	zoo_state_init(&g_restored);
	g_restored.d_io = &g_io.parent;
	g_restored.d_video = &g_video;
	g_restored.func_draw_sidebar = zoo_draw_sidebar_classic;
	handle = g_io.func_open_file_absolute(&g_io, argv[2], MODE_READ);
	if (handle.p == NULL) {
		fprintf(stderr, "failed to reopen the saved world\n");
		if (handle.func_close != NULL) handle.func_close(&handle);
		return 1;
	}
	result = zoo_world_load(&g_restored, &handle, false);
	if (handle.func_close != NULL) handle.func_close(&handle);
	if (result != 0 || !g_restored.world.info.is_save) {
		fprintf(stderr, "saved game did not reload as a SAV (result=%d)\n", result);
		return 1;
	}
	if (save_state(&g_restored, argv[3])) {
		fprintf(stderr, "re-saved world failed\n");
		return 1;
	}
	if (!files_equal(argv[2], argv[3])) {
		fprintf(stderr, "SAV load/save round trip was not byte-identical\n");
		return 1;
	}
	printf("PASS: Win32 file driver loaded DEMO.ZZT, redrew all 80x25 cells, matched title-sidebar text and attributes, and preserved a .SAV byte-for-byte across load/save.\n");
	return 0;
}
