package com.kerry.zzt;

/** Exhaustively compares the Android attribute adapter with VIDEO.PAS. */
public final class VideoAttributesCompat {
    public static void main(String[] args) {
        for (int attribute = 0; attribute <= 0xff; attribute++) {
            require(VideoAttributes.forDisplay(attribute, false) == attribute,
                    "Color mode changed attribute 0x" + hex(attribute) + ".");
            require(VideoAttributes.forDisplay(attribute, true) == pascalMonochrome(attribute),
                    "Monochrome mode differed from VIDEO.PAS for 0x" + hex(attribute) + ".");
        }

        require(VideoAttributes.forDisplay(0x00, true) == 0x70,
                "Black foreground did not become black on gray.");
        require(VideoAttributes.forDisplay(0x07, true) == 0x07,
                "Normal visible foreground did not become gray on black.");
        require(VideoAttributes.forDisplay(0x08, true) == 0x0f,
                "Bright foreground on black did not become white on black.");
        require(VideoAttributes.forDisplay(0x1f, true) == 0x7f,
                "Bright foreground on a colored background did not become white on gray.");
        require(VideoAttributes.forDisplay(0x87, true) == 0x07,
                "Monochrome conversion did not discard the color-mode blink bit.");

        System.out.println("Video attributes: all 256 color attributes and Pascal monochrome conversion pass");
    }

    private static int pascalMonochrome(int color) {
        if ((color & 0x08) != 0) {
            return (color & 0xf0) == 0 ? 0x0f : 0x7f;
        }
        return (color & 0x07) != 0 ? 0x07 : 0x70;
    }

    private static String hex(int value) {
        return String.format(java.util.Locale.ROOT, "%02x", value);
    }

    private static void require(boolean condition, String message) {
        if (!condition) throw new IllegalStateException(message);
    }
}
