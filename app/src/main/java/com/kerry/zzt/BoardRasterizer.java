package com.kerry.zzt;

/** Converts ZZT's 60x25 CP437 character and attribute buffer into native pixels. */
final class BoardRasterizer {
    static final int COLS = 60;
    static final int ROWS = 25;
    static final int CELL_W = 8;
    static final int CELL_H = 14;
    static final int WIDTH = COLS * CELL_W;
    static final int HEIGHT = ROWS * CELL_H;

    private static final int[] VGA = {
        0xff000000, 0xff0000aa, 0xff00aa00, 0xff00aaaa,
        0xffaa0000, 0xffaa00aa, 0xffaa5500, 0xffaaaaaa,
        0xff555555, 0xff5555ff, 0xff55ff55, 0xff55ffff,
        0xffff5555, 0xffff55ff, 0xffffff55, 0xffffffff
    };

    private BoardRasterizer() {}

    static void rasterize(byte[] source, byte[] font, int[] pixels,
                          boolean monochrome, long blinkAtMillis) {
        for (int cy = 0; cy < ROWS; cy++) {
            for (int cx = 0; cx < COLS; cx++) {
                int pos = (cy * COLS + cx) * 2;
                int ch = source[pos] & 0xff;
                int attr = VideoAttributes.forDisplay(source[pos + 1] & 0xff, monochrome);
                int fg = attr & 0x0f;
                int bg = (attr >> 4) & 0x0f;
                if ((attr & 0x80) != 0) {
                    bg &= 7;
                    if ((blinkAtMillis % 1000) < 500) fg = bg;
                }
                int foreground = VGA[fg];
                int background = VGA[bg];
                int glyph = ch * CELL_H;
                for (int gy = 0; gy < CELL_H; gy++) {
                    int bits = glyph + gy < font.length ? font[glyph + gy] & 0xff : 0;
                    int rowStart = (cy * CELL_H + gy) * WIDTH + cx * CELL_W;
                    for (int gx = 0; gx < CELL_W; gx++) {
                        pixels[rowStart + gx] = (bits & (0x80 >> gx)) != 0
                                ? foreground : background;
                    }
                }
            }
        }
    }
}
