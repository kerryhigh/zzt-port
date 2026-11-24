package com.kerry.zzt;

import java.nio.charset.Charset;

/** Host-side pixel regression for the Android 60x25 CP437 renderer. */
public final class BoardRasterizerTest {
    private static final int[] VGA = {
        0xff000000, 0xff0000aa, 0xff00aa00, 0xff00aaaa,
        0xffaa0000, 0xffaa00aa, 0xffaa5500, 0xffaaaaaa,
        0xff555555, 0xff5555ff, 0xff55ff55, 0xff55ffff,
        0xffff5555, 0xffff55ff, 0xffffff55, 0xffffffff
    };

    private BoardRasterizerTest() {}

    public static void main(String[] args) {
        require(args.length == 1, "Expected ZOO_KEY_CP437_BASE from the libzoo header");
        int engineCp437Base;
        try {
            engineCp437Base = Integer.parseInt(args[0]);
        } catch (NumberFormatException failure) {
            throw new AssertionError("Invalid ZOO_KEY_CP437_BASE argument", failure);
        }
        verifyEveryCp437IndexAndColorAttribute();
        verifyPascalBlinkPhases();
        verifyPascalMonochromeMapping();
        verifyCp437KeyboardMapping(engineCp437Base);
        System.out.println("PASS: Android 60x25 raster and DOS keyboard mapping cover all 256 CP437 bytes, colors, blink phases, and Pascal monochrome rules");
    }

    private static void verifyEveryCp437IndexAndColorAttribute() {
        byte[] frame = new byte[BoardRasterizer.COLS * BoardRasterizer.ROWS * 2];
        byte[] font = new byte[256 * BoardRasterizer.CELL_H];
        int[] pixels = new int[BoardRasterizer.WIDTH * BoardRasterizer.HEIGHT];

        for (int ch = 0; ch < 256; ch++) {
            for (int row = 0; row < BoardRasterizer.CELL_H; row++) {
                font[ch * BoardRasterizer.CELL_H + row] = (byte) syntheticGlyphRow(ch, row);
            }
            frame[ch * 2] = (byte) ch;
            frame[ch * 2 + 1] = (byte) ch;
        }

        BoardRasterizer.rasterize(frame, font, pixels, false, 500);
        for (int ch = 0; ch < 256; ch++) {
            int attr = ch;
            int fg = attr & 0x0f;
            int bg = (attr >> 4) & 0x0f;
            if ((attr & 0x80) != 0) bg &= 7;
            int cellX = ch % BoardRasterizer.COLS;
            int cellY = ch / BoardRasterizer.COLS;
            for (int gy = 0; gy < BoardRasterizer.CELL_H; gy++) {
                int bits = syntheticGlyphRow(ch, gy);
                for (int gx = 0; gx < BoardRasterizer.CELL_W; gx++) {
                    int actual = pixelAt(pixels, cellX, cellY, gx, gy);
                    int expected = (bits & (0x80 >> gx)) != 0 ? VGA[fg] : VGA[bg];
                    require(actual == expected,
                            "CP437 glyph/attribute mismatch at byte " + ch
                                    + " row " + gy + " column " + gx);
                }
            }
        }
    }

    private static void verifyPascalBlinkPhases() {
        byte[] frame = new byte[BoardRasterizer.COLS * BoardRasterizer.ROWS * 2];
        byte[] font = new byte[256 * BoardRasterizer.CELL_H];
        int[] pixels = new int[BoardRasterizer.WIDTH * BoardRasterizer.HEIGHT];
        frame[0] = (byte) 0xff;
        frame[1] = (byte) 0x9e;
        font[0xff * BoardRasterizer.CELL_H] = (byte) 0x80;

        BoardRasterizer.rasterize(frame, font, pixels, false, 0);
        require(pixels[0] == VGA[1] && pixels[1] == VGA[1],
                "Blink-off phase did not hide foreground against the low-intensity background");

        BoardRasterizer.rasterize(frame, font, pixels, false, 500);
        require(pixels[0] == VGA[14] && pixels[1] == VGA[1],
                "Blink-on phase did not restore the bright foreground");
    }

    private static void verifyPascalMonochromeMapping() {
        for (int attribute = 0; attribute < 256; attribute++) {
            int expected;
            if ((attribute & 0x08) != 0) {
                expected = (attribute & 0xf0) == 0 ? 0x0f : 0x7f;
            } else {
                expected = (attribute & 0x07) != 0 ? 0x07 : 0x70;
            }
            int actual = VideoAttributes.forDisplay(attribute, true);
            require(actual == expected,
                    "Pascal monochrome attribute mismatch at byte " + attribute);
        }
    }

    private static void verifyCp437KeyboardMapping(int engineCp437Base) {
        Charset cp437 = Charset.forName("IBM437");
        for (int value = 0; value < 256; value++) {
            String decoded = new String(new byte[] {(byte) value}, cp437);
            int codePoint = decoded.codePointAt(0);
            int expectedKey = value < 127 ? value : engineCp437Base + value;
            int expectedTextKey = value < 32 ? 0 : expectedKey;
            int actualKey = DosKeyboardMapping.keyForByte(value);
            int actual = DosKeyboardMapping.fromUnicode(codePoint);
            require(actualKey == expectedKey,
                    "CP437 byte-to-key mismatch for byte " + value);
            require(actual == expectedTextKey,
                    "CP437 keyboard round trip mismatch for byte " + value
                            + " (U+" + Integer.toHexString(codePoint) + ")");
        }
        require(DosKeyboardMapping.fromUnicode(0x20ac) == 0,
                "An unmappable Euro sign produced a DOS key");
        require(DosKeyboardMapping.fromUnicode(0x1f600) == 0,
                "An unmappable supplementary character produced a DOS key");
        require(DosKeyboardMapping.keyForByte(-1) == 0
                        && DosKeyboardMapping.keyForByte(256) == 0,
                "Out-of-range CP437 bytes produced DOS keys");
    }

    private static int syntheticGlyphRow(int character, int row) {
        return ((character * 31 + row * 13) ^ (0xa5 >>> (row & 7))) & 0xff;
    }

    private static int pixelAt(int[] pixels, int cellX, int cellY, int x, int y) {
        int pixelX = cellX * BoardRasterizer.CELL_W + x;
        int pixelY = cellY * BoardRasterizer.CELL_H + y;
        return pixels[pixelY * BoardRasterizer.WIDTH + pixelX];
    }

    private static void require(boolean condition, String message) {
        if (!condition) throw new AssertionError(message);
    }
}
